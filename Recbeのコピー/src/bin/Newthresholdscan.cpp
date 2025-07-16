/***************************************
*                                      *
* Threshold Scan for RECBE             *
*                                      *
* threshold_scan_v1.7                  *
*                                      *
* 2025/07/13 Update                    *
* - sleep time reduced to 5ms          *
* - total runtime printed as h:m:s     *
* - FG output turned ON at start       *
*                                      *
***************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#define SOFTWARE_VERSION "threshold_scan_v1.7"

// Constants
const int UDP_PORT = 4660;
const int TCP_PORT = 24;
const int HEADER_SIZE = 12;
const int NUM_EVENT = 300;
const int RANGE_MIN = 3350;
const int RANGE_MAX = 3850;
const int DAC_STEP = 1;

struct rbcp_header {
    uint8_t type;
    uint8_t command;
    uint8_t id;
    uint8_t length;
    uint32_t address;
};

struct header {
    uint8_t type;
    uint8_t ver;
    uint16_t id;
    uint16_t time;
    uint16_t length;
    uint32_t trgCount;
};

void createDir(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        if (mkdir(path.c_str(), 0755) != 0) {
            std::cerr << "Failed to create directory: " << path << std::endl;
            exit(1);
        }
    }
}

void rbcp_write(int sock, const std::string &ip, int udp_port, uint32_t addr, uint8_t data) {
    std::cout << "[DEBUG] Preparing UDP write to " << ip << " port " << udp_port
              << " addr=0x" << std::hex << addr << " data=0x" << std::hex << (int)data << std::endl;

    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(udp_port);
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());

    rbcp_header hdr{};
    hdr.type = 0xFF;
    hdr.command = 0x80;
    hdr.id = 1;
    hdr.length = 1;
    hdr.address = htonl(addr);

    char packet[2048] = {};
    memcpy(packet, &hdr, sizeof(rbcp_header));
    packet[sizeof(rbcp_header)] = data;

    ssize_t sent = sendto(sock, packet, sizeof(rbcp_header) + 1, 0,
                          (struct sockaddr *) &servaddr, sizeof(servaddr));

    if (sent < 0) {
        std::cerr << "[ERROR] sendto() failed. errno=" << errno
                  << " (" << strerror(errno) << ")" << std::endl;
        exit(1);
    } else {
        std::cout << "[DEBUG] sendto() returned " << sent << std::endl;
    }
}

void set_tdc_threshold(int sock, const std::string &ip, int udp_port, uint16_t threshold) {
    std::cout << "[DEBUG] set_tdc_threshold called with value 0x" << std::hex << threshold << std::endl;
    uint8_t upper = (threshold >> 8) & 0xFF;
    uint8_t lower = threshold & 0xFF;
    rbcp_write(sock, ip, udp_port, 0x1c, upper);
    rbcp_write(sock, ip, udp_port, 0x1d, lower);
    usleep(5000); // 5ms
    std::cout << "[DEBUG] set_tdc_threshold completed" << std::endl;
}

void run_acquisition(int sock,
                     int num_event,
                     const std::string &dat_file) {

    std::cout << "[DEBUG] Starting run_acquisition()" << std::endl;

    std::ofstream ofs(dat_file, std::ios::binary);
    if (!ofs) {
        std::cerr << "[ERROR] Failed to open file: " << dat_file << std::endl;
        close(sock);
        exit(1);
    }

    for (int ev = 0; ev < num_event; ++ev) {
        unsigned char header_buf[HEADER_SIZE];
        int read_bytes = 0;

        while (read_bytes < HEADER_SIZE) {
            int n = read(sock, header_buf + read_bytes, HEADER_SIZE - read_bytes);
            if (n <= 0) {
                std::cerr << "[ERROR] read() error in header read." << std::endl;
                close(sock);
                exit(1);
            }
            read_bytes += n;
        }

        ofs.write((char *) header_buf, HEADER_SIZE);

        header *hdr = (header *) header_buf;
        uint16_t len = ntohs(hdr->length);

        std::vector<unsigned char> data(len);
        read_bytes = 0;
        while (read_bytes < len) {
            int n = read(sock, data.data() + read_bytes, len - read_bytes);
            if (n <= 0) {
                std::cerr << "[ERROR] read() error in payload read." << std::endl;
                close(sock);
                exit(1);
            }
            read_bytes += n;
        }

        ofs.write((char *) data.data(), len);
    }

    std::cout << "[DEBUG] run_acquisition() completed" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "[INFO] Software version: " << SOFTWARE_VERSION << std::endl;

    if (argc != 4) {
        std::cerr << "Usage: ./threshold_scan <SNo> <FST_CH> <IP>\n";
        return 1;
    }

    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    std::string SNo = argv[1];
    int FST_CH = atoi(argv[2]);
    std::string IP = argv[3];

    int NUM_CH = 8;
    int LAST_CH = FST_CH + NUM_CH - 1;

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        std::cerr << "[ERROR] Failed to create UDP socket\n";
        return 1;
    }

    // TCP接続
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        std::cerr << "[ERROR] Failed to create TCP socket\n";
        return 1;
    }
    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    servaddr.sin_addr.s_addr = inet_addr(IP.c_str());

    std::cout << "[DEBUG] Attempting TCP connect to " << IP << ":" << TCP_PORT << std::endl;
    int ret = connect(tcp_sock, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret < 0) {
        std::cerr << "[ERROR] connect() failed. errno=" << errno
                  << " (" << strerror(errno) << ")" << std::endl;
        close(tcp_sock);
        return 1;
    }
    std::cout << "[DEBUG] connect() successful to " << IP << ":" << TCP_PORT << std::endl;

    // FG output ON
    std::cout << "[DEBUG] Turning ON FG_control output" << std::endl;
    system("./FG_control 1 -s outputSwitch on");
    system("./FG_control 2 -s outputSwitch on");

    for (int fg_mv = 10; fg_mv <= 50; fg_mv += 10) {

        std::cout << "[DEBUG] Starting FG_control for fg_mv = " << fg_mv << std::endl;

        double amplitude_V = fg_mv / 1000.0;
        double offset_V = amplitude_V / -2.0;

        std::ostringstream cmd1;
        cmd1 << "./FG_control 1 -s amplitude " << amplitude_V;
        std::cout << "[DEBUG] Running: " << cmd1.str() << std::endl;
        int ret1 = system(cmd1.str().c_str());
        std::cout << "[DEBUG] FG_control amplitude return code: " << ret1 << std::endl;

        std::ostringstream cmd2;
        cmd2 << "./FG_control 1 -s offset " << offset_V;
        std::cout << "[DEBUG] Running: " << cmd2.str() << std::endl;
        int ret2 = system(cmd2.str().c_str());
        std::cout << "[DEBUG] FG_control offset return code: " << ret2 << std::endl;

        for (int vth_mV = RANGE_MIN; vth_mV <= RANGE_MAX; vth_mV += DAC_STEP) {

            std::cout << "[DEBUG] Threshold loop vth_mV = 0x" << std::hex << vth_mV << std::endl;

            set_tdc_threshold(udp_sock, IP, UDP_PORT, vth_mV);

            std::ostringstream dat_dir;
            dat_dir << "../dat/" << SNo << "/thresholdscan";
            createDir("../dat/" + SNo);
            createDir(dat_dir.str());

            std::ostringstream filename_prefix;
            filename_prefix << FST_CH << "_" << LAST_CH
                            << "_FG" << fg_mv << "mV_DAC" << vth_mV << "mV";
            std::string dat_file = dat_dir.str() + "/" + filename_prefix.str() + ".dat";

            run_acquisition(tcp_sock, NUM_EVENT, dat_file);

            std::ostringstream root_dir;
            root_dir << "../ROOT/" << SNo << "/thresholdscan";
            createDir("../ROOT/" + SNo);
            createDir(root_dir.str());
            createDir(root_dir.str() + "/fig");
            createDir(root_dir.str() + "/fitting");

            std::ostringstream root_file;
            root_file << root_dir.str() << "/" << filename_prefix.str() << ".root";

            std::ostringstream conv_cmd;
            conv_cmd << "./binary2root2 " << dat_file << " " << root_file.str();
            std::cout << "[DEBUG] Running: " << conv_cmd.str() << std::endl;
            int conv_ret = system(conv_cmd.str().c_str());
            std::cout << "[DEBUG] binary2root2 return code: " << conv_ret << std::endl;
        }

        std::ostringstream ts_cmd;
        ts_cmd << "./thresholdscan " << SNo << " " << fg_mv << " " << FST_CH;
        std::cout << "[DEBUG] Running: " << ts_cmd.str() << std::endl;
        int ts_ret = system(ts_cmd.str().c_str());
        std::cout << "[DEBUG] thresholdscan return code: " << ts_ret << std::endl;
    }

    close(tcp_sock);
    close(udp_sock);

    std::cout << "[DEBUG] Turning off FG_control output" << std::endl;
    system("./FG_control 1 -s outputSwitch off");
    system("./FG_control 2 -s outputSwitch off");

    gettimeofday(&end_time, nullptr);
    long seconds = end_time.tv_sec - start_time.tv_sec;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int sec = seconds % 60;
    std::cout << "[INFO] Total runtime: "
              << hours << "h "
              << minutes << "m "
              << sec << "s" << std::endl;

    return 0;
}