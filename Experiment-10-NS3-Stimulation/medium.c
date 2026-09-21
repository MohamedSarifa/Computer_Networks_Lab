#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 50000
#define BOTTLE_QUEUE_MAX 20 // Equivalent to StringValue ("20p")
#define MSS 1460            // Maximum Segment Size in Bytes

// Event types for the discrete event scheduler
typedef enum {
    EVENT_SEND,       // Sender sends a packet
    EVENT_ARRIVE_ROUTER, // Packet reaches the bottleneck router
    EVENT_ARRIVE_SINK,   // Packet reaches the receiver destination
    EVENT_ARRIVE_ACK     // ACK reaches back to the sender
} event_type_t;

typedef struct {
    double time;
    event_type_t type;
    uint32_t seq_num;
    uint32_t packet_size;
} Event;

// Event Queue State
Event event_queue[MAX_EVENTS];
int event_count = 0;

// Network Topology Parameters
const double LINK1_DELAY = 0.005;  // 5ms
const double LINK2_DELAY = 0.020;  // 20ms
const double LINK1_BANDWIDTH = 10000000.0; // 10Mbps
const double LINK2_BANDWIDTH = 2000000.0;  // 2Mbps (Bottleneck)

// Router Queue State
uint32_t router_queue[BOTTLE_QUEUE_MAX];
int router_queue_count = 0;
double router_next_available_tx_time = 0.0;

// TCP NewReno Sender State Machine Variables
double cwnd = 1460.0;       // Initial Congestion Window (1 MSS in bytes)
double ssthresh = 65535.0;  // Slow Start Threshold
uint32_t next_seq = 0;
uint32_t highest_ack = 0;
int dup_ack_count = 0;

// Performance Counters
uint32_t total_tx_packets = 0;
uint32_t total_rx_packets = 0;
uint32_t total_lost_packets = 0;
double total_end_to_end_delay = 0.0;

// File pointer for tracking metrics
FILE *cwnd_file;

// Event Scheduler Functions
void schedule_event(double time, event_type_t type, uint32_t seq_num, uint32_t size) {
    if (event_count >= MAX_EVENTS) return;
    
    // Insert event keeping the array sorted by chronological time execution order
    int i = event_count - 1;
    while (i >= 0 && event_queue[i].time > time) {
        event_queue[i + 1] = event_queue[i];
        i--;
    }
    event_queue[i + 1].time = time;
    event_queue[i + 1].type = type;
    event_queue[i + 1].seq_num = seq_num;
    event_queue[i + 1].packet_size = size;
    event_count++;
}

// TCP NewReno Window Event Processor
void process_ack_newreno(double current_time, uint32_t ack_num) {
    if (ack_num > highest_ack) {
        highest_ack = ack_num;
        dup_ack_count = 0;
        
        // Congestion Window Growth Check
        if (cwnd < ssthresh) {
            // Slow Start Phase: Exponential growth
            cwnd += MSS; 
        } else {
            // Congestion Avoidance Phase: Linear growth (+1 MSS per RTT approximation)
            cwnd += (double)(MSS * MSS) / cwnd;
        }
        
        // Proactively pipeline more data based on updated window size boundaries
        while ((next_seq - highest_ack) + MSS <= cwnd) {
            schedule_event(current_time, EVENT_SEND, next_seq, MSS);
            next_seq += MSS;
        }
    } else if (ack_num == highest_ack) {
        dup_ack_count++;
        // Fast Retransmit Trigger upon Triple Duplicate ACKs
        if (dup_ack_count == 3) {
            total_lost_packets++;
            ssthresh = cwnd / 2.0;
            if (ssthresh < MSS) ssthresh = MSS;
            cwnd = ssthresh; // Multiplicative Decrease (NewReno fallback structural state)
            
            // Retransmit missing packet segment immediately
            schedule_event(current_time, EVENT_SEND, ack_num, MSS);
        }
    }
    
    // Log the current state matching the NS-3 CwndTracer output format
    fprintf(cwnd_file, "%.6f\t%u\n", current_time, (uint32_t)cwnd);
}

int main() {
    cwnd_file = fopen("cwnd_data.dat", "w");
    if (!cwnd_file) {
        printf("Error opening output data logging trace file!\n");
        return 1;
    }

    // Initialize Simulation: Start Bulk Sender app transmission stream at 1.0s (Matches App Start Time)
    double sim_time = 1.0;
    schedule_event(sim_time, EVENT_SEND, next_seq, MSS);
    next_seq += MSS;

    // Main Simulation Loop Core Processor
    while (event_count > 0 && sim_time <= 10.0) {
        // Extract the earliest pending chronological event from queue
        Event current_event = event_queue[0];
        for (int i = 0; i < event_count - 1; i++) {
            event_queue[i] = event_queue[i + 1];
        }
        event_count--;
        
        sim_time = current_event.time;
        if (sim_time > 10.0) break; // Hard stop at 10.0 seconds

        switch (current_event.type) {
            case EVENT_SEND: {
                total_tx_packets++;
                // Transmission time over Link 1 (Size in bits / Bandwidth)
                double tx_time = (current_event.packet_size * 8.0) / LINK1_BANDWIDTH;
                double arrival_at_router = sim_time + tx_time + LINK1_DELAY;
                schedule_event(arrival_at_router, EVENT_ARRIVE_ROUTER, current_event.seq_num, current_event.packet_size);
                break;
            }
            case EVENT_ARRIVE_ROUTER: {
                // Check DropTail queue limitations on the bottleneck router link
                if (router_queue_count >= BOTTLE_QUEUE_MAX) {
                    // Queue Full: Packet Drop Event occurs
                    // No event is scheduled, causing a timeout or duplicate ACK pipeline later
                    total_lost_packets++;
                } else {
                    // Enqueue packet
                    router_queue[router_queue_count++] = current_event.seq_num;
                    
                    // Process Router Link 2 Serialization and Transmission delays
                    if (sim_time > router_next_available_tx_time) {
                        router_next_available_tx_time = sim_time;
                    }
                    
                    double tx_time_link2 = (current_event.packet_size * 8.0) / LINK2_BANDWIDTH;
                    double arrival_at_sink = router_next_available_tx_time + tx_time_link2 + LINK2_DELAY;
                    
                    schedule_event(arrival_at_sink, EVENT_ARRIVE_SINK, current_event.seq_num, current_event.packet_size);
                    
                    // Lock router transmission resource interface line
                    router_next_available_tx_time += tx_time_link2;
                    router_queue_count--; // Dequeue tracking optimization
                }
                break;
            }
            case EVENT_ARRIVE_SINK: {
                total_rx_packets++;
                // Record End-to-End delays (Approximate baseline metric matching FlowMonitor)
                total_end_to_end_delay += (sim_time - (1.0 + (double)current_event.seq_num / LINK1_BANDWIDTH));
                
                // Receiver returns structural network acknowledgment (ACK) instantly back to source
                double ack_arrival_time = sim_time + LINK2_DELAY + LINK1_DELAY;
                schedule_event(ack_arrival_time, EVENT_ARRIVE_ACK, current_event.seq_num + MSS, 40); // 40-byte TCP ACK
                break;
            }
            case EVENT_ARRIVE_ACK: {
                process_ack_newreno(sim_time, current_event.seq_num);
                break;
            }
        }
    }

    // 9. Extract and Evaluate Performance Metrics Console Summary
    double simulation_window = 9.0; // From 1.0s to 10.0s 
    double throughput_mbps = ((total_rx_packets * MSS * 8.0) / simulation_window) / 1000000.0;
    double pdr = ((double)total_rx_packets / (double)total_tx_packets) * 100.0;
    double avg_delay = total_end_to_end_delay / total_rx_packets;

    printf("\n======= PERFORMANCE METRICS (PURE C SIMULATION) =======\n");
    printf("Flow: 10.1.1.1 -> 10.1.2.2\n");
    printf("Throughput: %.4f Mbps\n", throughput_mbps);
    printf("Packet Delivery Ratio (PDR): %.2f %%\n", pdr);
    printf("Average End-to-End Delay: %.6f s\n", avg_delay);
    printf("Total Packets Transmitted: %u\n", total_tx_packets);
    printf("Total Packets Received: %u\n", total_rx_packets);
    printf("Total Packets Lost: %u\n", total_lost_packets);
    printf("========================================================\n");

    fclose(cwnd_file);
    return 0;
}

