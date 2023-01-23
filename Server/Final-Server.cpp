//Name: Jane Feng

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

extern int port = 8080;
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

// convert message into a binary string
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
        for (unsigned int i = 1; i < unsigned(binary.size()); i++) {
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

// Single-Server / Multi-Client
int main() {
    // Initialize Winsock
    WSADATA wsaData;

    int initial_win;
    initial_win = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (initial_win != 0) {
        printf("Fail to initialize WinSock \n");
        return error;
    }

    // Create a Socket
    SOCKET server_socket = INVALID_SOCKET; // Create a server socket object
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == INVALID_SOCKET) {
        printf("Socket cannot be created \n");
        WSACleanup(); // End Winsock
        return error;
    }

    // Bind Socket to Port and IP Address 
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.S_un.S_addr = INADDR_ANY; // Bind to any address

    int binding;
    binding = bind(server_socket, (SOCKADDR*)&address, sizeof(address));

    if (binding == SOCKET_ERROR) {
        printf("Binding failed \n");
        WSACleanup(); // End Winsock
        return error;
    }

    // Listen / Wait for Connection
    // SOMAXCONN sets backlog to the maximum # of connections in queue possible
    int listening;
    listening = listen(server_socket, SOMAXCONN);

    if (listening == SOCKET_ERROR) {
        printf("Listening failed \n");
        WSACleanup(); // End Winsock
        return error;
    }

    printf("Listening ... \n");

    // Single-Server / Multi-Client
    fd_set master; // master set
    FD_ZERO(&master); // zero the master set

    FD_SET(server_socket, &master);

    int buffer_size = 0;

    while (true) {
        fd_set copy_master = master; // creating a copy of master set
        
        int numsocket = select(0, &copy_master, nullptr, nullptr, nullptr);

        for (int i = 0; i < numsocket; i++) {
            SOCKET sock = copy_master.fd_array[i];
            
            if (sock == server_socket) { // check if the sock is an incoming communication request from client
                // accept connection from incoming client
                SOCKET client_socket = accept(server_socket, nullptr, nullptr);

                FD_SET(client_socket, &master); // add connection to our master set 

                printf("Connected to Client \n");
                
                // Performing 3-Way Handshake - Connect to Client
                struct handshake { // Struct to hold the info we want to send during handshake
                    int protocol;
                    int seq;
                    int syn;
                    int ack;
                    int buffer_size;
                };
    
                char test_buffer[length]; // test_buffer[1024]
    
                // receive SYN
                //int test_receive = recv(client_socket, test_buffer, test_length, 0);
                recv(client_socket, test_buffer, length, 0);

                // Save the seq, syn, ack, and buffer_size received (SYN packet) 
                handshake received_syn;
                memcpy(&received_syn, test_buffer, sizeof(handshake));

                // check that packet client sent is the SYN packet (protocol # is 1)
                if (received_syn.protocol != 1) {
                    for (int i = 0; i <= 3; i++) {
            
                        if (i >= 3) {
                            printf("Error. Closing Connection \n");
                            WSACleanup();
                            return error;
                        }
            
                        printf("Error. Please retransmit SYN packet \n");
            
                        // create ERROR packet to request for retransmission, set everything to 0
                        handshake error_packet; 
                        error_packet.protocol = 4; // protocol # 4 for error

                        // Save ERROR packet struct into test_buffer
                        memcpy(test_buffer, &error_packet, sizeof(handshake));

                        // send the ERROR packet
                        send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

                        recv(client_socket, test_buffer, length, 0);
        
                        // Save the seq, syn, ack, and buffer_size received (retransmitted SYN packet) 
                        handshake received_syn;
                        memcpy(&received_syn, test_buffer, sizeof(handshake));

                        if (received_syn.protocol == 1) {
                            break;
                        }
                    }
                }

                //std::cout << "testing " << std::string(test_buffer, 0, ) << std::endl;

                // Create SYN-ACK packet
                handshake server_handshake;
                server_handshake.protocol = 2; // Set value to 2 for SYN-ACK packet
                server_handshake.seq = 1234; // server ISN
                server_handshake.syn = 1; // Set value to 1 because we want to perform SYNchronization
                server_handshake.ack = received_syn.seq + 1; // Add 1 to seq received to ACKnowledge 

                // Save server_handshake struct into test_buffer
                memcpy(test_buffer, &server_handshake, sizeof(handshake));

                // send the SYN-ACK packet
                send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

                // response from client (ACK packet or ERROR packet)
                recv(client_socket, test_buffer, length, 0);

                // Save the response packet received (if protocol # = 3, ACK packet, if protocol # = 4 or != 3, ERROR packet)
                handshake received_ack;
                memcpy(&received_ack, test_buffer, sizeof(handshake));

                // check if packet received is ACK packet (protocol # is 3)
                if ((received_ack.protocol == 4) || (received_ack.protocol != 3)) {
                    for (int i = 0; i <= 3; i++) {

                        if (i >= 3) {
                            printf("Error. Closing Connection \n");
                            WSACleanup();
                            return error;
                        }

                        if (received_ack.protocol == 4) {
                            printf("Retransmitting SYN-ACK packet \n");

                            // Save server_handshake struct into test_buffer (SYN-ACK packet)
                            memcpy(test_buffer, &server_handshake, sizeof(handshake));

                            // send SYN-ACK packet 
                            send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);
                        }

                        else if (received_ack.protocol != 3) {
                            printf("Please retransmit ACK packet \n");
                
                            // create ERROR packet to request for retransmission, set everything to 0
                            handshake error_packet;
                            error_packet.protocol = 4; // protocol # 4 for error

                            // Save ERROR packet struct into test_buffer
                            memcpy(test_buffer, &error_packet, sizeof(handshake));

                            // send the ERROR packet
                            send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);
                        }
            
                        // response from server (ACK packet or ERROR packet)
                        recv(client_socket, test_buffer, length, 0);

                        // Save response packet received 
                        handshake received_ack;
                        memcpy(&received_ack, test_buffer, sizeof(handshake));

                        //(if protocol # = 3, ACK packet)
                        if (received_ack.protocol == 3) {
                            break;
                        }

                    }
                }

                // Check if client acknowledge, if not ask for retransmission
                if (received_ack.ack != server_handshake.seq + 1) {
                    for (int i = 0; i <= 3; i++) {

                        printf("Error. Client did not acknowledge. Please retransmit ACK packet \n");

                        do {
                            // create ERROR packet to request for retransmission
                            handshake error_packet;
                            error_packet.protocol = 4; // protocol # 4 for error

                            // Save ERROR packet struct into test_buffer
                            memcpy(test_buffer, &error_packet, sizeof(handshake));

                            // send the ERROR packet
                            send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

                            // response from server (ACK packet)
                            recv(client_socket, test_buffer, length, 0);

                            // Save response packet received 
                            handshake received_ack;
                            memcpy(&received_ack, test_buffer, sizeof(handshake));

                            if (received_ack.ack == server_handshake.seq + 1) {
                                break;
                            }
                        } while (received_ack.ack != server_handshake.seq + 1);

                        if (i >= 3) {
                            printf("Client did not acknowledge. Closing Connection \n");
                            WSACleanup();
                            return error;
                        }
                    }
                }

                else {
                    handshake handshake_complete;
                    handshake_complete.protocol = 0;

                    // Save HANDSHAKE_COMPLETE packet struct into test_buffer
                    memcpy(test_buffer, &handshake_complete, sizeof(handshake));

                    // send the HANDSHAKE_COMPLETE packet
                    send(client_socket, (char*)&test_buffer, sizeof(struct handshake), 0);

                    if (handshake_complete.protocol != 0) {
                        printf("Handshake Failed. Closing Connection \n");
                        WSACleanup();
                        return error;
                    }

                    buffer_size = received_syn.buffer_size;
                    printf("Handshake Completed \n\n");
                }
            }

            else { // accept the incoming data

                // Transmitting Data 
                // Create new char* buffer with agreed buffer size
                int exchanged_buff_size = buffer_size;
                char* buffer = NULL;
                buffer = (char*)malloc((exchanged_buff_size + 1) * sizeof(char*));

                if (buffer == NULL) {
                    printf("Error \n");
                    WSACleanup();
                    return error;
                }

                int data_receive = 0; // Variable to store the # of bytes received

                // store the number of bytes received 
                data_receive = recv(sock, buffer, exchanged_buff_size, 0);

                if (data_receive == 0) { // If no bytes are received, end the connection
                    // drop client
                    closesocket(sock); // close the socket
                    FD_CLR(sock, &master); // removing from the master set
                }

                else { 

                    std::string data_packet = buffer; // data packet
                    std::string protocol = data_packet.substr(0, 1); // index 0 = protocol #
                    std::string ack = data_packet.substr(1, 1); // index 1 = ack
                    std::string received_checksum = data_packet.substr(2, 8); // index 2 - 9 = checksum
                    std::string message = data_packet.substr(10); // index 10 - end of packet = message

                    // Retransmitting Data
                    if (protocol != "5") { // if protocol # is not 5 
                        for (int i = 0; i <= 3; i++) {
                            std::string data_error = "";

                            printf("Error. Please retransmit data packet \n");
                            data_error.append("7"); // set to 7 for data transmission error
                            data_error.append("0");

                            memcpy(buffer, data_error.c_str(), data_error.size() + 1);
                            send(sock, buffer, data_error.size() + 1, 0);

                            // store the number of bytes received 
                            data_receive = recv(sock, buffer, exchanged_buff_size, 0);

                            std::string data_packet = buffer;
                            std::string protocol = data_packet.substr(0, 1); // index 0 = protocol #
                            std::string ack = data_packet.substr(1, 1); // index 1 = ack
                            std::string received_checksum = data_packet.substr(2, 8); // index 2 - 9 = checksum
                            std::string message = data_packet.substr(10); // index 10 - end of packet = message

                            if (protocol == "5") {
                                break;
                            }

                            if (i >= 3) {
                                data_error.append("8");
                                data_error.append("0");
                                memcpy(buffer, data_error.c_str(), data_error.size() + 1);
                                send(sock, buffer, data_error.size() + 1, 0);
                                printf("Data Transmission Failed \n");
                                WSACleanup();
                                return error;
                            }
                        }
                    }

                    if (protocol == "5") { // data packet protocol # is 5
                        std::string checksum = get_checksum(message); // calculate the checksum of message client sent

                        if (checksum != received_checksum) { // if calculated checksum isn't the same as the checksum from client, retransmit data
                            // for loop for counter, after 3 retransmission requests, if it still doesn't receieve the correct checksum end connection
                            for (int i = 0; i <= 3; i++) {
                                std::string data_error = "";

                                printf("Checksum Error. Please retransmit data packet \n");
                                data_error.append("7"); // set to 7 for data transmission error
                                data_error.append("0");

                                memcpy(buffer, data_error.c_str(), data_error.size() + 1);
                                send(sock, buffer, data_error.size() + 1, 0);

                                // store the number of bytes received 
                                data_receive = recv(sock, buffer, exchanged_buff_size, 0);

                                std::string data_packet = buffer;
                                std::string protocol = data_packet.substr(0, 1); // index 0 = protocol #
                                std::string ack = data_packet.substr(1, 1); // index 1 = ack
                                std::string received_checksum = data_packet.substr(2, 8); // index 2 - 9 = checksum
                                std::string message = data_packet.substr(10); // index 10 - end of packet = message

                                std::string checksum = get_checksum(message);

                                if (checksum == received_checksum) {
                                    break;
                                }

                                if (i >= 3) {
                                    data_error.append("8");
                                    data_error.append("0");
                                    memcpy(buffer, data_error.c_str(), data_error.size() + 1);
                                    send(sock, buffer, data_error.size() + 1, 0);
                                    printf("Data Transmission Failed \n");
                                    WSACleanup();
                                    return error;
                                }
                            }
                        }

                        printf("Data Received: \n");
                        std::string transmission_success = "";
                        transmission_success.append("6");
                        transmission_success.append("1"); // set ack to 1 if data transmission is successful

                        memcpy(buffer, transmission_success.c_str(), transmission_success.size() + 1);
                        send(sock, buffer, transmission_success.size() + 1, 0);

                        std::cout << "checksum: " << received_checksum << std::endl;
                        std::cout << "message: " << message << std::endl;
                        std::cout << "message length: " << message.length() << std::endl << std::endl;
                    }

                    data_receive = 0; // reset the # of bytes received
                }
            }
        }
    }

    // End Winsock
    WSACleanup();

    return 0;
}

