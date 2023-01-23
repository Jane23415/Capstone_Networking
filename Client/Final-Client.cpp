//Name: Jane Feng

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

extern int port = 8080; // Listening port
extern const int length = 1024; // buffer length
extern int error = 1;


// Checksum - Functions
// Function for adding the binary sequences
void sum_function(std::string checksum, std::string temp, char carry, std::string* sum_temp, char* carry_temp) {
    std::string zero = "0";
    std::string one = "1";
    std::string sum = ""; // string to hold the sum 
    char carry_value = carry; // store the given carry value

    int index = checksum.length() - 1; // get the starting index 

    while (index > -1) { // while loop until index is below 0
        // checks what the value at each index of current checksum + temp string and carry value is
        // inserts the sum of the 3 values if satifies the 3 conditions into sum string
        if (checksum[index] == '0' && temp[index] == '0' && carry_value == '0') {
            sum.insert(0, zero);
            carry_value = '0';
        }
        else if (checksum[index] == '0' && temp[index] == '0' && carry_value == '1') {
            sum.insert(0, one);
            carry_value = '0';
        }
        else if (((checksum[index] == '0' && temp[index] == '1') || (checksum[index] == '1' && temp[index] == '0')) && carry_value == '0') {
            sum.insert(0, one);
            carry_value = '0';
        }
        else if (((checksum[index] == '0' && temp[index] == '1') || (checksum[index] == '1' && temp[index] == '0')) && carry_value == '1') {
            sum.insert(0, zero);
            carry_value = '1';
        }
        else if (checksum[index] == '1' && temp[index] == '1' && carry_value == '0') {
            sum.insert(0, zero);
            carry_value = '1';
        }
        else if (checksum[index] == '1' && temp[index] == '1' && carry_value == '1') {
            sum.insert(0, one);
            carry_value = '1';
        }
        index--; // decrease index 
    }

    if (carry_value == '1') { // After the while loop, if carry_value is = 1, insert 1 to the beginning of the sum string 
        sum.insert(0, one);
        carry_value = '0'; // set carry value back to 0
    }

    // returning our sum and carry_value
    *sum_temp = sum;
    *carry_temp = carry_value;
}

// convert message into a binary sequence
std::string get_checksum(std::string message) {
    int length = message.length();
    std::vector<std::string> binary; // store the converted binary sequence 
    std::string zero = "0";
    std::string one = "1";
    for (int i = 0; i < length; i++) {
        int ascii_value = int(message[i]); // convert each char in the message into its ASCII value
        std::string temp = ""; // stores the binary for each char

        while (ascii_value > 0) { // converting the ASCII value into its binary equivalent 
            //int temp_index = temp.length();
            if (ascii_value % 2 == 0) { // if the ASCII value is even, add a 0 to the front of the string
                temp.insert(0, zero);
            }
            else {
                temp.insert(0, one); // if ASCII value is odd, add a 1 to the front of the string
            }
            ascii_value = ascii_value / 2; // divide ASCII value in half until its lower than 0
        }

        if ((temp.length() % 8) != 0) { // checks thats all the sequence are 8 bits long, if not insert 0 to beginning of it until length = 8
            int num_zero = 8 - (temp.length() % 8);
            for (int i = 0; i < num_zero; i++) {
                temp.insert(0, zero);
            }
        }

        binary.push_back(temp); // append the binary sequence for each char
    }

    char carry = '0'; // set carry to 0
    std::string checksum = binary[0]; // set checksum to the first sequence in the binary vector

    if (binary.size() > 1) { // if the binary vector has more than 1 sequence
        for (int i = 1; i < binary.size(); i++) {
            std::string temp = binary[i]; // save sequence at index i in temp 
            std::string sum;

            sum_function(checksum, temp, carry, &sum, &carry); // compute the sum of the binary sequence 

            while (sum.length() > 8) { // if the length of sum is greater than 8 
                int index = sum.length() % 8;
                temp = sum.substr(0, index);
                if ((temp.length() % 8) != 0) {
                    int num_zero = 8 - (temp.length() % 8);
                    for (int i = 0; i < num_zero; i++) {
                        temp.insert(0, zero);
                    }
                }
                checksum = sum.substr(index, sum.length());
                sum_function(checksum, temp, carry, &sum, &carry);
            }

            checksum = sum;
            if ((checksum.length() % 8) != 0) {
                int num_zero = 8 - (checksum.length() % 8);
                for (int i = 0; i < num_zero; i++) {
                    checksum.insert(0, zero);
                }
            }
        }
    }

    return checksum;
}

int main() {

    std::string ip_address = "127.0.0.1"; // IP Address

    // Initialize WinSock
    WSADATA wsaData;

    int initial_win;
    initial_win = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (initial_win != 0) {
        printf("Fail to initialize WinSock \n");
        return error;
    }

    // Create Socket
    SOCKET client_socket = INVALID_SOCKET;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == INVALID_SOCKET) {
        printf("Socket cannot be created \n");
        WSACleanup(); // End Winsock
        return error;
    }


    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr); // Convert IP Address from a string to binary form 

    int address_size = sizeof(address);

    // Connect to Server
    int connecting;
    connecting = connect(client_socket, (SOCKADDR*)&address, address_size);

    if (connecting == SOCKET_ERROR) {
        printf("Connection failed \n");
        closesocket(client_socket);
        WSACleanup(); // End Winsock
        return error;
    }

    printf("Connected to Server \n");

    // Performing 3_Way Handshake - Connect to Server 
    struct handshake { // Struct to hold the info we want to send during handshake
        int protocol;
        int seq;
        int syn;
        int ack;
        int buffer_size;
    };

    char test_buffer[length]; // test_buffer[1024] 

    // Create SYN packet
    handshake client_syn;
    client_syn.protocol = 1; // Set value to 1 for SYN packet
    client_syn.seq = 4321; // client ISN 
    client_syn.syn = 1; // Set value to 1 because we want to perform SYNchronization
    client_syn.ack = 0; // Set value to 0 b/c we haven't acknowledge connection yet
    client_syn.buffer_size = 1024; // Buffer size 

    // Save client_handshake struct into test_buffer 
    memcpy(test_buffer, &client_syn, sizeof(handshake));

    // send SYN packet 
    send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

    // response from server SYN-ACK packet or ERROR packet
    recv(client_socket, test_buffer, length, 0);

    // Save response packet received (if protocol # = 2, SYN-ACK packet, if protocol # = 4 or != 2, ERROR packet)
    handshake received_synack;
    memcpy(&received_synack, test_buffer, sizeof(handshake));

    // check if packet received is SYN-ACK packet
    if ((received_synack.protocol == 4) || (received_synack.protocol != 2)) {
        for (int i = 0; i <= 3; i++) {

            if (i >= 3) {
                printf("Error. Closing Connection \n");
                WSACleanup();
                return error;
            }

            if (received_synack.protocol == 4) {

                printf("Retransmitting SYN packet \n");

                // Save client_handshake struct into test_buffer (SYN packet)
                memcpy(test_buffer, &client_syn, sizeof(handshake));

                // send SYN packet 
                send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);
            }

            else if (received_synack.protocol != 2) {

                printf("Please retransmit SYN-ACK packet \n");
                // create ERROR packet to request for retransmission, set everything to 0
                handshake error_packet;
                error_packet.protocol = 4; // protocol # 4 for error

                // Save ERROR packet struct into test_buffer
                memcpy(test_buffer, &error_packet, sizeof(handshake));

                // send the ERROR packet
                send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);
            }

            // response from server (SYN-ACK packet or ERROR packet)
            recv(client_socket, test_buffer, length, 0);

            // Save response packet received 
            handshake received_synack;
            memcpy(&received_synack, test_buffer, sizeof(handshake));

            //(if protocol # = 2, SYN-ACK packet)
            if (received_synack.protocol == 2) {
                break;
            }

        }
    }

    // Check if server acknowledge, if not ask for retransmission 
    else if (received_synack.ack != client_syn.seq + 1) {
        for (int i = 0; i <= 3; i++) {

            printf("Error. Server did not acknowledge. Please retransmit SYN-ACK packet \n");

            do {
                // create ERROR packet to request for retransmission, set everything to 0
                handshake error_packet;
                error_packet.protocol = 4; // protocol # 4 for error

                // Save ERROR packet struct into test_buffer
                memcpy(test_buffer, &error_packet, sizeof(handshake));

                // send the ERROR packet
                send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

                // response from server (SYN-ACK packet)
                recv(client_socket, test_buffer, length, 0);

                // Save response packet received 
                handshake received_synack;
                memcpy(&received_synack, test_buffer, sizeof(handshake));

                if (received_synack.ack == client_syn.seq + 1) {
                    break;
                }
            } while (received_synack.ack != client_syn.seq + 1);

            if (i >= 3) {
                printf("Server did not acknowledge. Closing Connection \n");
                WSACleanup();
                return error;
            }
        }
    }

    // Create ACK packet
    handshake client_ack;
    client_ack.protocol = 3; // Set to 3 for ACK packet
    client_ack.seq = received_synack.ack;
    client_ack.syn = 0;
    client_ack.ack = received_synack.seq + 1;

    memcpy(test_buffer, &client_ack, sizeof(handshake));

    // send ACK
    send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

    // response from server HANDSHAKE_COMPLETE packet or ERROR packet
    recv(client_socket, test_buffer, length, 0);

    // Save response packet received (if protocol # = 0, HANDSHAKE_COMPLETE packet, if protocol # = 4 or != 0, ERROR packet)
    handshake handshake_complete;
    memcpy(&handshake_complete, test_buffer, sizeof(handshake));

    // check if packet received is HANDSHAKE_COMPLETE packet (protocol # is 0)
    if (handshake_complete.protocol == 4) {
        for (int i = 0; i <= 3; i++) {

            printf("Retransmitting ACK packet \n");

            // Save client_ack struct into test_buffer (ACK packet)
            memcpy(test_buffer, &client_ack, sizeof(handshake));

            // send ACK packet 
            send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

            // response from server (HANDSHAKE_COMPLETE packet or ERROR packet)
            recv(client_socket, test_buffer, length, 0);

            // Save response packet received 
            handshake handshake_complete;
            memcpy(&handshake_complete, test_buffer, sizeof(handshake));

            // if protocol # = 0, ACK packet
            if (handshake_complete.protocol == 0) {
                break;
            }

            if (i >= 3) {
                printf("Error sending ACK packet. Closing Connection \n");
                WSACleanup();
                return error;
            }
        }
    }

    else if (handshake_complete.protocol != 0) {
        printf("Handshake Failed. Closing Connection \n");
        WSACleanup();
        return error;
    }

    else {
        printf("Handshake Completed \n");
    }

    // Transmitting Data 
    // Create new char* buffer with agreed buffer size
    int exchanged_buff_size = client_syn.buffer_size; // 1024
    char* buffer;
    buffer = (char*)malloc((exchanged_buff_size + 1) * sizeof(char*));

    if (buffer == NULL) {
        printf("Error \n");
        WSACleanup();
        return error;
    }

    std::string input_message;

    do { // while user input message is not nothing send to server; if nothing end the connection

        std::cout << "Enter message (Press enter to exit): "; // ask user for message
        std::getline(std::cin, input_message); // input the entire string into message

        if (input_message.size() == 0) { // If message is nothing, end the connection 
            printf("Ending connection ... \n");
            break;
        }

        std::string checksum = get_checksum(input_message);
        //std::string checksum = "00000000";

        std::string data_packet = "";
        data_packet.append("5"); // protocol # (index 0), 5 for data transmission
        data_packet.append("0"); // ack (index 1), initialize to 0
        data_packet.append(checksum); // checksum (index 2 - 9)
        data_packet.append(input_message); // message (index 10 and so on) 


        memcpy(buffer, data_packet.c_str(), sizeof(data_packet)); // store the message in binary form into buffer
        send(client_socket, buffer, data_packet.size() + 1, 0);

        // store the number of bytes received 
        int server_data = recv(client_socket, buffer, exchanged_buff_size, 0);

        std::string server_packet = buffer;
        std::string protocol = server_packet.substr(0, 1);
        std::string ack = server_packet.substr(1, 1);

        // Retransmitting Data
        if ((protocol == "6" && ack == "0") || (protocol == "7") || server_data == 0) {
            for (int i = 0; i <= 3; i++) {
                printf("Retransmitting Data \n");

                memcpy(buffer, data_packet.c_str(), sizeof(data_packet)); // store the message in binary form into buffer
                send(client_socket, buffer, data_packet.size() + 1, 0);

                server_data = recv(client_socket, buffer, exchanged_buff_size, 0);

                std::string server_packet = buffer;
                std::string protocol = server_packet.substr(0, 1);
                std::string ack = server_packet.substr(1, 1);

                if (protocol == "6" && ack == "1") {
                    break;
                }

                else if ((protocol == "8") || (i >= 3)) {
                    printf("Data Transmission Failed \n");
                    WSACleanup();
                    return error;
                }
            }
        }

        if (protocol == "6" && ack == "1") {
            printf("Data Transmission Successful \n");
        }

    } while (input_message.size() > 0);

    // Close Socket 
    closesocket(client_socket);

    // End WinSock
    WSACleanup();

    return 0;
}