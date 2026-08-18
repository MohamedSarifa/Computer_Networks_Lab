#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>

const int MAX_NODES = 10;
const int MAX_LEN = 4096;

typedef struct {
    char url[64];
    char ip[16];
    char mac[18];
} Node;

typedef struct {
    char name[20];
    unsigned short hex_code;
} PPPProtocol;

PPPProtocol ppp_table[] = {
    {"IP",   0x0021},
    {"IPv6",  0x0057},
    {"LCP",   0xC021},
    {"PAP",   0xC023},
    {"CHAP",  0xC223},
    {"IPCP",  0x8021},
    {"CCP",   0x80FD}
};

int total_protocols = sizeof(ppp_table) / sizeof(ppp_table[0]);
Node network_table[10];
int node_count = 0;

void init_network_table() {
    strcpy(network_table[0].url, "google.com");
    strcpy(network_table[0].ip, "142.250.190.46");
    strcpy(network_table[0].mac, "AA:EE:CC:56:67:33");

    strcpy(network_table[1].url, "client.lan");
    strcpy(network_table[1].ip, "192.168.1.10");
    strcpy(network_table[1].mac, "AA:BB:CC:11:22:33");

    node_count = 2;
}

int find_node(const char *url) {
    for (int i = 0; i < node_count; i++) {
        if (strcmp(network_table[i].url, url) == 0) return i;
    }
    return -1;
}

void hex16_to_binary(unsigned short hex_val, char *bin_out) {
    int pos = 0;
    for (int b = 15; b >= 0; b--) {
        bin_out[pos++] = ((hex_val >> b) & 1) ? '1' : '0';
        if (b == 8) {
            bin_out[pos++] = ' ';
        }
    }
    bin_out[pos] = '\0';
}

void str_to_binary(const char *input, char *binary_out) {
    binary_out[0] = '\0';
    for (int i = 0; input[i] != '\0'; i++) {
        unsigned char c = input[i];
        for (int b = 7; b >= 0; b--) {
            strcat(binary_out, ((c >> b) & 1) ? "1" : "0");
        }
        if (input[i + 1] != '\0') strcat(binary_out, " ");
    }
}

void mac_to_binary(const char *mac, char *bin_out) {
    bin_out[0] = '\0';
    for (int i = 0; mac[i] != '\0'; i++) {
        if (mac[i] == ':') continue;
        int val;
        sscanf(&mac[i], "%1x", &val);
        for (int b = 3; b >= 0; b--) {
            strcat(bin_out, ((val >> b) & 1) ? "1" : "0");
        }
        if (i % 2 != 0 && mac[i + 1] != '\0') strcat(bin_out, " ");
    }
}

void ip_to_binary(const char *ip, char *bin_out) {
    bin_out[0] = '\0';
    int octets[4];
    sscanf(ip, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]);
    for (int i = 0; i < 4; i++) {
        for (int b = 7; b >= 0; b--) {
            strcat(bin_out, ((octets[i] >> b) & 1) ? "1" : "0");
        }
        if (i < 3) strcat(bin_out, " ");
    }
}

void perform_byte_stuffing(const char *msg, char *stuffed_bin) {
    char header_prefix[] = "10101010 10111011 11001100 00010001 00100010 00110011 10101010 11101110 11001100 01010110 01100111 00110011 00001000 00001000 00001000 00001000 00001000 00001000 00001000 00001000 ";
    strcpy(stuffed_bin, header_prefix);

    for (int i = 0; msg[i] != '\0'; i++) {
        unsigned char c = msg[i];
        if (c == 0x7E) {
            strcat(stuffed_bin, "01111101 01111110 ");
        } else {
            for (int b = 7; b >= 0; b--) {
                char bit[2] = {((c >> b) & 1) ? '1' : '0', '\0'};
                strcat(stuffed_bin, bit);
            }
            strcat(stuffed_bin, " ");
        }
    }
    strcat(stuffed_bin, "00000000");
}

void ppp_checksum(const char raw_binary_message[], int wordSize, char *fcs_out_binary) {
    char message[8192] = "";
    int pos = 0;

    for (int i = 0; raw_binary_message[i] != '\0'; i++) {
        if (raw_binary_message[i] != ' ' && raw_binary_message[i] != '\n' && raw_binary_message[i] != '\r') {
            message[pos++] = raw_binary_message[i];
        }
    }
    message[pos] = '\0';

    int len = strlen(message);
    int rem = len % wordSize;

    if (rem != 0) {
        int pad = wordSize - rem;
        for (int i = 0; i < pad; i++) {
            strcat(message, "0");
        }
        len = strlen(message);
    }

    int words = len / wordSize;
    int sum[128] = {0};

    for (int i = 0; i < wordSize; i++) {
        sum[i] = message[i] - '0';
    }

    for (int i = 1; i < words; i++) {
        int carry = 0;
        for (int j = wordSize - 1; j >= 0; j--) {
            int bit = sum[j] + (message[i * wordSize + j] - '0') + carry;
            sum[j] = bit % 2;
            carry = bit / 2;
        }
        while (carry) {
            for (int j = wordSize - 1; j >= 0; j--) {
                int bit = sum[j] + carry;
                sum[j] = bit % 2;
                carry = bit / 2;
                if (carry == 0) break;
            }
        }
    }

    printf("\n--- PPP CHECKSUM COMPUTATION (Word Size: %d bits) ---", wordSize);
    printf("\nSender Side");
    printf("\n-----------");
    printf("\nFinal Sum : ");
    for (int i = 0; i < wordSize; i++) {
        printf("%d", sum[i]);
        if (wordSize == 16 && i == 7) printf(" ");
    }

    printf("\nChecksum  : ");
    int check[128];
    fcs_out_binary[0] = '\0';
    for (int i = 0; i < wordSize; i++) {
        check[i] = !sum[i];
        printf("%d", check[i]);
        char b[2] = {check[i] + '0', '\0'};
        strcat(fcs_out_binary, b);
        if (wordSize == 16 && i == 7) {
            printf(" ");
            strcat(fcs_out_binary, " ");
        }
    }
    printf("\n");
}

void display_ppp_table() {
    char bin_str[32];
    printf("\n========================================================================\n");
    printf("                PPP PROTOCOL LOOKUP TABLE                            \n");
    printf("========================================================================\n");
    printf("%-4s | %-12s | %-10s | %-19s\n", "ID", "Name", "Hex Code", "Binary Equivalent");
    printf("------------------------------------------------------------------------\n");
    for (int i = 0; i < total_protocols; i++) {
        hex16_to_binary(ppp_table[i].hex_code, bin_str);
        printf("%-4d | %-12s | 0x%04X     | %s\n",
               i + 1, ppp_table[i].name, ppp_table[i].hex_code, bin_str);
    }
    printf("========================================================================\n");
}

void generate_and_stream_frame() {
    char src_url[64], dst_url[64], filename[64];
    int packet_size, frame_size, wordSize;

    printf("\n--- FRAME CONFIGURATION WINDOW ---\n");
    printf("Source URL : ");
    scanf("%s", src_url);
    printf("Destination URL : ");
    scanf("%s", dst_url);
    printf("Packet Bit Size (e.g. 32 bits) : ");
    scanf("%d", &packet_size);
    printf("Frame Bit Size (e.g. 10 bits) : ");
    scanf("%d", &frame_size);
    printf("Enter Checksum Word Size (8 or 16) : ");
    scanf("%d", &wordSize);

    display_ppp_table();

    char proto_input[32];
    PPPProtocol selected_proto = ppp_table[0];
    int found = 0;

    printf("\nEnter Protocol Name or ID (e.g., IP, LCP, 3): ");
    scanf("%s", proto_input);

    if (isdigit(proto_input[0])) {
        int idx = atoi(proto_input) - 1;
        if (idx >= 0 && idx < total_protocols) {
            selected_proto = ppp_table[idx];
            found = 1;
        }
    }

    if (!found) {
        for (int i = 0; i < total_protocols; i++) {
            if (strcmp(proto_input, ppp_table[i].name) == 0) {
                selected_proto = ppp_table[i];
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        printf("[!] Invalid Protocol selection. Defaulting to IP (0x0021).\n");
    }

    char proto_binary[32];
    hex16_to_binary(selected_proto.hex_code, proto_binary);

    printf("Source File Name : ");
    scanf("%s", filename);

    int src_idx = find_node(src_url);
    int dst_idx = find_node(dst_url);

    if (src_idx == -1 || dst_idx == -1) {
        printf("\n[ERROR] Routing conflict: Match paths not verified.\n");
        return;
    }

    FILE *fp = fopen(filename, "r");
    char message[256] = "~H";
    if (fp) {
        if (fgets(message, sizeof(message), fp) != NULL) {
            message[strcspn(message, "\r\n")] = 0;
        }
        fclose(fp);
    }

    printf("========================================================================\n");
    printf(" LAYER 1 : APPLICATION LAYER\n");
    printf("------------------------------------------------------------------------\n");
    printf("  Message         : %s\n", message);
    char app_binary[4096];
    str_to_binary(message, app_binary);
    printf("  Binary Format   : %s\n", app_binary);
    printf("\n========================================================================\n");

    printf("========================================================================\n");
    printf(" LAYER 2 : TRANSPORT LAYER (SEGMENT)\n");
    printf("------------------------------------------------------------------------\n");
    printf("  Source Port              : 43690\n");
    printf("  Destination Port         : 50054\n");
    printf("------------------------------------------------------------------------\n");
    printf(" Binary Format\n");
    printf(" Source Port               : 10101010 10101010\n");
    printf(" Destination Port          : 11000011 10000110\n");
    printf("------------------------------------------------------------------------\n");
    printf(" Layer Output:\n");
    printf("10101010 10101010 11000011 10000110 %s\n", app_binary);
    printf("========================================================================\n");

    char src_ip_bin[64], dst_ip_bin[64];
    ip_to_binary(network_table[src_idx].ip, src_ip_bin);
    ip_to_binary(network_table[dst_idx].ip, dst_ip_bin);

    printf("========================================================================\n");
    printf(" LAYER 3 : NETWORK LAYER (PACKET FRAGMENTATION)\n");
    printf("----------------------------------------------------------------------\n");
    printf("  [ Packet 1 ]\n");
    printf("  Source IP        : %s\n", network_table[src_idx].ip);
    printf("  Destination IP   : %s\n", network_table[dst_idx].ip);
    printf("----------------------------------------------------------------------\n");
    printf(" Packet Binary Output:\n");
    printf("%s %s 10101010 10101010 11000011 10000110 %s\n\n", src_ip_bin, dst_ip_bin, app_binary);
    printf(" Combined Fragmented Stream:\n");
    printf(" %s %s 10101010 10101010 11000011 10000110 %s\n", src_ip_bin, dst_ip_bin, app_binary);
    printf("========================================================================\n");

    printf("\n==============================================================\n");
    printf(" LAYER 4 : DATA LINK LAYER (FRAMING PROCESS)\n");
    printf("==============================================================\n");

    char raw_app_bin[4096] = "";
    int pos = 0;
    for (int i = 0; app_binary[i] != '\0'; i++) {
        if (app_binary[i] != ' ') {
            raw_app_bin[pos++] = app_binary[i];
        }
    }
    raw_app_bin[pos] = '\0';

    int total_payload_bits = strlen(raw_app_bin);
    int num_frames = (total_payload_bits + frame_size - 1) / frame_size;

    printf("\n--------------------------------------------------------------\n");
    printf(" >>> PACKET 1 (Contains %d Bits) <<<\n", total_payload_bits);
    printf("--------------------------------------------------------------\n");

    char frame_payloads[64][1024];
    int frame_paddings[64];

    for (int f = 0; f < num_frames; f++) {
        int start_bit = f * frame_size;
        int bits_left = total_payload_bits - start_bit;
        int current_payload_bits = (bits_left >= frame_size) ? frame_size : bits_left;
        int padding_bits = frame_size - current_payload_bits;

        strncpy(frame_payloads[f], raw_app_bin + start_bit, current_payload_bits);
        frame_payloads[f][current_payload_bits] = '\0';
        frame_paddings[f] = padding_bits;

        printf("\n  [ Frame %d ]\n", f + 1);
        printf("  Source MAC      : %s\n", network_table[src_idx].mac);
        printf("  Destination MAC : %s\n", network_table[dst_idx].mac);
        printf("  Payload Bits    : %d\n", current_payload_bits);
        printf("  Payload Text    : %s\n", frame_payloads[f]);
        printf("  Padding Bits    : %d\n", padding_bits);
        if (padding_bits > 0) {
            printf("  Padding Content : ");
            for (int p = 0; p < padding_bits; p++) putchar('0');
            putchar('\n');
        }
        printf("  Error Control   : 00000000\n");
    }

    char src_mac_bin[128], dst_mac_bin[128];
    mac_to_binary(network_table[src_idx].mac, src_mac_bin);
    mac_to_binary(network_table[dst_idx].mac, dst_mac_bin);

    printf("\n==============================================================\n");
    printf(" COMPLETE BINARY FRAME STREAM OUTPUT\n");
    printf("==============================================================\n\n");

    for (int f = 0; f < num_frames; f++) {
        printf("Frame %d Stream: %s %s %s %s %s",
               f + 1, src_mac_bin, dst_mac_bin, src_ip_bin, dst_ip_bin,
               "10101010 10101010 11000011 10000110 ");

        printf("%s", frame_payloads[f]);
        for (int p = 0; p < frame_paddings[f]; p++) {
            putchar('0');
        }
        printf(" 00000000\n");
    }

    char byte_stuffed_info[4096];
    perform_byte_stuffing(message, byte_stuffed_info);

    char full_ppp_payload[8192] = "1111111100000011";
    char temp_proto[32] = "";
    for (int i = 0; proto_binary[i] != '\0'; i++) {
        if (proto_binary[i] != ' ') {
            char b[2] = {proto_binary[i], '\0'};
            strcat(temp_proto, b);
        }
    }
    strcat(full_ppp_payload, temp_proto);

    char temp_info[4096] = "";
    for (int i = 0; byte_stuffed_info[i] != '\0'; i++) {
        if (byte_stuffed_info[i] != ' ') {
            char b[2] = {byte_stuffed_info[i], '\0'};
            strcat(temp_info, b);
        }
    }
    strcat(full_ppp_payload, temp_info);

    char calculated_fcs[128];
    ppp_checksum(full_ppp_payload, wordSize, calculated_fcs);
    printf("\n========================================================\n");
    printf("                  PPP FRAME FORMAT (SENDER)\n");
    printf("========================================================\n\n");
    printf("Flag\n-----\n01111110\n\n");
    printf("Address\n-------\n11111111\n\n");
    printf("Control\n-------\n00000011\n\n");
    printf("Protocol (%s - 0x%04X)\n--------\n%s\n\n", selected_proto.name, selected_proto.hex_code, proto_binary);
    printf("Information (Data)\n-------------------\n");
    printf("%s\n\n", byte_stuffed_info);
    printf("FCS (Calculated Checksum)\n--------------------------\n%s\n\n", calculated_fcs);
    printf("Flag\n-----\n01111110\n");

    char err_choice[10];
    printf("\nDo you want to introduce transmission error? (y/n): ");
    scanf("%s", err_choice);
    if (err_choice[0] == 'y' || err_choice[0] == 'Y') {
        int bit_pos;
        printf("Enter bit position to modify (0 to %lu): ", strlen(full_ppp_payload) - 1);
        scanf("%d", &bit_pos);
        if (bit_pos >= 0 && bit_pos < (int)strlen(full_ppp_payload)) {
            full_ppp_payload[bit_pos] = (full_ppp_payload[bit_pos] == '1') ? '0' : '1';
            printf("[!] Bit flipped at position %d inside binary payload\n", bit_pos);
        }
    }

    char tx_addr[16] = "11111111";
    char tx_ctrl[16] = "00000011";
    char tx_proto[32] = "";
    char tx_info[4096] = "";

    strncpy(tx_addr, full_ppp_payload, 8); tx_addr[8] = '\0';
    strncpy(tx_ctrl, full_ppp_payload + 8, 8); tx_ctrl[8] = '\0';

    snprintf(tx_proto, sizeof(tx_proto), "%.8s %.8s", full_ppp_payload + 16, full_ppp_payload + 24);

    int info_len = strlen(full_ppp_payload + 32);
    pos = 0;
    for (int i = 0; i < info_len; i++) {
        tx_info[pos++] = full_ppp_payload[32 + i];
        if ((i + 1) % 8 == 0 && i != info_len - 1) {
            tx_info[pos++] = ' ';
        }
    }
    tx_info[pos] = '\0';

    char transmitted_frame[8192];
    snprintf(transmitted_frame, sizeof(transmitted_frame),
             "01111110|%s|%s|%s|%s|%s|01111110|%d|%s",
             tx_addr, tx_ctrl, tx_proto, tx_info, calculated_fcs, wordSize, message);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("[ERROR] Socket creation failed.\n");
        return;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("[ERROR] Connection to server failed.\n");
        close(sock);
        return;
    }

    send(sock, transmitted_frame, strlen(transmitted_frame), 0);

    char response[1024] = {0};
    recv(sock, response, sizeof(response), 0);
    printf("\nServer Response: %s\n", response);

    close(sock);
}

int main() {
    init_network_table();
    int choice;

    while (1) {
        printf("\n=================== NETWORK TOOL MENU ===================\n");
        printf(" 1. Add Network Node\n");
        printf(" 2. Delete Network Node\n");
        printf(" 3. Generate & Stream Frame\n");
        printf(" 4. View Network Table\n");
        printf(" 5. Exit Program\n");
        printf("---------------------------------------------------------\n");
        printf("Enter your choice (1-5): ");
        if (scanf("%d", &choice) != 1) break;

        switch (choice) {
            case 3:
                generate_and_stream_frame();
                break;
            case 5:
                printf("\nExiting program. Goodbye!\n");
                return 0;
            default:
                break;
        }
    }
    return 0;
}
