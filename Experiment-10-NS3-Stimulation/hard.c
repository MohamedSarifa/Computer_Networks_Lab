#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#define MAX_EVENTS 200000
#define BOTTLE_QUEUE_MAX 25  // Queue limit equivalent to 25p
#define MSS 1460             // Maximum Segment Size in Bytes
#define SIM_END_TIME 15.0

// Event structural types matching NS-3 triggers
typedef enum {
    EVENT_SEND_FLOW1,
    EVENT_SEND_FLOW2,
    EVENT_ARRIVE_ROUTER,
    EVENT_ARRIVE_SINK,
    EVENT_ARRIVE_ACK
} event_t;

typedef struct {
    double time;
    event_t type;
    int flow_id;
    uint32_t seq_num;
    uint32_t size;
} SimEvent;

// Global Event Queue Parameters
SimEvent event_queue[MAX_EVENTS];
int event_count = 0;

// Topological Channel Parameters
const double ACCESS_DELAY = 0.005;     // 5ms
const double BOTTLENECK_DELAY = 0.030; // 30ms
const double ACCESS_BW = 10000000.0;   // 10Mbps
const double BOTTLENECK_BW = 2000000.0; // 2Mbps Shared Bottleneck
const double PACKET_ERROR_RATE = 0.0005; // 0.05% loss simulation profile

// Router Queue Infrastructure Tracking
uint32_t router_queue_slots[BOTTLE_QUEUE_MAX];
int router_queue_count = 0;
double router_next_tx_free_time = 0.0;

// TCP Protocol State Variables (Flow 1: NewReno, Flow 2: Cubic)
typedef struct {
    double cwnd;
    double ssthresh;
    uint32_t next_seq;
    uint32_t highest_ack;
    int dup_ack_count;
    
    // Cubic Unique State Markers
    double epoch_start;
    double w_max;
    double K;
    double origin_cwnd;
} TcpState;

TcpState tcp_flows[2];

// Metrics Trackers
uint32_t tx_packets[2] = {0, 0};
uint32_t rx_packets[2] = {0, 0};
uint32_t lost_packets[2] = {0, 0};
double delay_sum[2] = {0.0, 0.0};

FILE *f_reno, *f_cubic;

// Chronological Event Priority Insertion
void schedule_event(double time, event_t type, int flow_id, uint32_t seq, uint32_t size) {
    if (event_count >= MAX_EVENTS) return;
    int i = event_count - 1;
    while (i >= 0 && event_queue[i].time > time) {
        event_queue[i + 1] = event_queue[i];
        i--;
    }
    event_queue[i + 1].time = time;
    event_queue[i + 1].type = type;
    event_queue[i + 1].flow_id = flow_id;
    event_queue[i + 1].seq_num = seq;
    event_queue[i + 1].size = size;
    event_count++;
}

// TCP NewReno State Logic (Flow 0)
void handle_newreno_ack(double current_time, uint32_t ack_num) {
    TcpState *s = &tcp_flows[0];
    if (ack_num > s->highest_ack) {
        s->highest_ack = ack_num;
        s->dup_ack_count = 0;
        if (s->cwnd < s->ssthresh) {
            s->cwnd += MSS; // Slow Start
        } else {
            s->cwnd += (double)(MSS * MSS) / s->cwnd; // Additive Increase
        }
        while ((s->next_seq - s->highest_ack) + MSS <= s->cwnd) {
            schedule_event(current_time, EVENT_SEND_FLOW1, 0, s->next_seq, MSS);
            s->next_seq += MSS;
        }
    } else if (ack_num == s->highest_ack) {
        s->dup_ack_count++;
        if (s->dup_ack_count == 3) { // Multiplicative Decrease Trigger
            lost_packets[0]++;
            s->ssthresh = s->cwnd / 2.0;
            if (s->ssthresh < 2 * MSS) s->ssthresh = 2 * MSS;
            s->cwnd = s->ssthresh;
            schedule_event(current_time, EVENT_SEND_FLOW1, 0, ack_num, MSS);
        }
    }
    fprintf(f_reno, "%.6f\t%u\n", current_time, (uint32_t)s->cwnd);
}

// TCP Cubic State Logic (Flow 1)
void handle_cubic_ack(double current_time, uint32_t ack_num) {
    TcpState *s = &tcp_flows[1];
    const double C = 0.4;
    const double beta = 0.7;

    if (ack_num > s->highest_ack) {
        s->highest_ack = ack_num;
        s->dup_ack_count = 0;

        if (s->cwnd < s->ssthresh) {
            s->cwnd += MSS; // Standard Slow Start Behavior
        } else {
            // Cubic Window Core Equation: W(t) = C*(t - K)^3 + W_max
            if (s->epoch_start == 0.0) {
                s->epoch_start = current_time;
                if (s->cwnd < s->w_max) {
                    s->K = cbrt((s->w_max - s->cwnd) / C);
                } else {
                    s->K = 0;
                }
                s->origin_cwnd = s->cwnd;
            }
            double t = current_time - s->epoch_start + s->K;
            double target = C * pow(t - s->K, 3) + s->w_max;
            
            // Limit increment window bounding per ACK reception
            if (target > s->cwnd) {
                s->cwnd += ((target - s->cwnd) * MSS) / s->cwnd;
            } else {
                s->cwnd += (double)(MSS * MSS) / s->cwnd;
            }
        }
        while ((s->next_seq - s->highest_ack) + MSS <= s->cwnd) {
            schedule_event(current_time, EVENT_SEND_FLOW2, 1, s->next_seq, MSS);
            s->next_seq += MSS;
        }
    } else if (ack_num == s->highest_ack) {
        s->dup_ack_count++;
        if (s->dup_ack_count == 3) { // Cubic Dynamic Window Reduction
            lost_packets[1]++;
            s->epoch_start = 0.0; // Reset Cubic window structural clock
            if (s->cwnd < s->w_max) {
                s->w_max = s->cwnd * (1.0 + beta) / 2.0; // Fast convergence optimization
            } else {
                s->w_max = s->cwnd;
            }
            s->ssthresh = s->cwnd * beta;
            if (s->ssthresh < 2 * MSS) s->ssthresh = 2 * MSS;
            s->cwnd = s->cwnd * beta;
            schedule_event(current_time, EVENT_SEND_FLOW2, 1, ack_num, MSS);
        }
    }
    fprintf(f_cubic, "%.6f\t%u\n", current_time, (uint32_t)s->cwnd);
}

int main() {
    f_reno = fopen("cwnd_newreno.dat", "w");
    f_cubic = fopen("cwnd_cubic.dat", "w");
    srand(42); // Pin seed configuration for statistical consistency

    // Initialize Default State Parameters
    for(int i=0; i<2; i++) {
        tcp_flows[i].cwnd = 10 * MSS; // Initial window boost matching modern networks
        tcp_flows[i].ssthresh = 65535;
        tcp_flows[i].next_seq = 0;
        tcp_flows[i].highest_ack = 0;
        tcp_flows[i].w_max = 0;
        tcp_flows[i].epoch_start = 0.0;
    }

    // Schedule Parallel Start Sequences at 1.0 seconds
    double sim_time = 1.0;
    schedule_event(sim_time, EVENT_SEND_FLOW1, 0, tcp_flows[0].next_seq, MSS);
    tcp_flows[0].next_seq += MSS;
    schedule_event(sim_time, EVENT_SEND_FLOW2, 1, tcp_flows[1].next_seq, MSS);
    tcp_flows[1].next_seq += MSS;

    // Simulation Event Processing Loop
    while (event_count > 0 && sim_time <= SIM_END_TIME) {
        SimEvent curr = event_queue[0];
        for (int i = 0; i < event_count - 1; i++) event_queue[i] = event_queue[i + 1];
        event_count--;

        sim_time = curr.time;
        if (sim_time > SIM_END_TIME) break;

        switch (curr.type) {
            case EVENT_SEND_FLOW1:
            case EVENT_SEND_FLOW2: {
                int fid = curr.flow_id;
                tx_packets[fid]++;
                double tx_delay = (curr.size * 8.0) / ACCESS_BW;
                schedule_event(sim_time + tx_delay + ACCESS_DELAY, EVENT_ARRIVE_ROUTER, fid, curr.seq_num, curr.size);
                break;
            }
            case EVENT_ARRIVE_ROUTER: {
                int fid = curr.flow_id;
                // Evaluate physical random packet error profile
                double rand_val = (double)rand() / RAND_MAX;
                if (rand_val < PACKET_ERROR_RATE) {
                    lost_packets[fid]++; // Random structural degradation drop
                    break;
                }

                if (router_queue_count >= BOTTLE_QUEUE_MAX) {
                    lost_packets[fid]++; // Buffer congestion drop
                } else {
                    router_queue_slots[router_queue_count++] = curr.seq_num;
                    if (sim_time > router_next_tx_free_time) router_next_tx_free_time = sim_time;
                    
                    double b_tx_delay = (curr.size * 8.0) / BOTTLENECK_BW;
                    double destination_arrival = router_next_tx_free_time + b_tx_delay + BOTTLENECK_DELAY;
                    
                    schedule_event(destination_arrival, EVENT_ARRIVE_SINK, fid, curr.seq_num, curr.size);
                    router_next_tx_free_time += b_tx_delay;
                    router_queue_count--;
                }
                break;
            }
            case EVENT_ARRIVE_SINK: {
                int fid = curr.flow_id;
                rx_packets[fid]++;
                delay_sum[fid] += (sim_time - (1.0 + (double)curr.seq_num / ACCESS_BW));
                
                double ack_return_time = sim_time + BOTTLENECK_DELAY + ACCESS_DELAY;
                schedule_event(ack_return_time, EVENT_ARRIVE_ACK, fid, curr.seq_num + MSS, 40);
                break;
            }
            case EVENT_ARRIVE_ACK: {
                if (curr.flow_id == 0) handle_newreno_ack(sim_time, curr.seq_num);
                else handle_cubic_ack(sim_time, curr.seq_num);
                break;
            }
        }
    }

    // Evaluation Metric Printouts
    double runtime = SIM_END_TIME - 1.0;
    double t_reno = ((rx_packets[0] * MSS * 8.0) / runtime) / 1e6;
    double t_cubic = ((rx_packets[1] * MSS * 8.0) / runtime) / 1e6;
    
    // Jain's Fairness Index Calculation Formula
    double fairness = pow(t_reno + t_cubic, 2) / (2 * (pow(t_reno, 2) + pow(t_cubic, 2)));

    printf("\n================= SIMULATION COMPARATIVE DATA =================\n");
    printf(">>> FLOW 1: TCP NewReno\n");
    printf("    Throughput  : %.4f Mbps\n", t_reno);
    printf("    Avg Delay   : %.2f ms\n", (delay_sum[0] / rx_packets[0]) * 1000);
    printf("    Packets Lost: %u\n", lost_packets[0]);
    
    printf("\n>>> FLOW 2: TCP Cubic\n");
    printf("    Throughput  : %.4f Mbps\n", t_cubic);
    printf("    Avg Delay   : %.2f ms\n", (delay_sum[1] / rx_packets[1]) * 1000);
    printf("    Packets Lost: %u\n", lost_packets[1]);
    
    printf("\n---------------------------------------------------------------\n");
    printf("Jain's Fairness Index Calculation Metric: %.4f\n", fairness);
    printf("===============================================================\n");
    fclose(f_reno); 
    fclose(f_cubic);
    return 0;}
