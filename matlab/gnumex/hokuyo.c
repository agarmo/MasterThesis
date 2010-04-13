
#include "hokuyo.h"


static int com_connect(const char* device, long baudrate) {

    char adjust_device[16];
    sprintf(adjust_device, "\\\\.\\%s", device);
    HComm = CreateFile(adjust_device, GENERIC_READ | GENERIC_WRITE, 0,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (HComm == INVALID_HANDLE_VALUE) {
        return -1;
    }

    // Baud Rate setting
    // !!! Not done

    return 0;
}


static void com_disconnect(void) {
    if (HComm != INVALID_HANDLE_VALUE) {
        CloseHandle(HComm);
        HComm = INVALID_HANDLE_VALUE;
    }
    printf("diconnected from COM\n");
}


static int com_send(const char* data, int size) {
    DWORD n;
    WriteFile(HComm, data, size, &n, NULL);
    return n;
}


static int com_recv(char* data, int max_size, int timeout) {

    int filled = 0;
    int readable_size = 0;

    // Reading of Concerned data.
    do {
        DWORD dwErrors;
        COMSTAT ComStat;
        ClearCommError(HComm, &dwErrors, &ComStat);
        readable_size = (int)ComStat.cbInQue;
        int read_n = (readable_size > max_size) ? max_size : readable_size;

        DWORD n;
        ReadFile(HComm, &data[filled], read_n, &n, NULL);
        filled += n;
        readable_size -= n;

        if (filled >= max_size) {
            return filled;
        }
    } while (readable_size > 0);

    if (timeout > 0) {
        // Read data using time out
        COMMTIMEOUTS pcto;
        GetCommTimeouts(HComm, &pcto);
        pcto.ReadIntervalTimeout = 0;
        pcto.ReadTotalTimeoutMultiplier = 0;
        pcto.ReadTotalTimeoutConstant = timeout;
        SetCommTimeouts(HComm, &pcto);

        //Read data one charaters each
        DWORD n;
        while (1) {
            ReadFile(HComm, &data[filled], 1, &n, NULL);
            if (n < 1) {
                return filled;
            }
            filled += n;
            if (filled >= max_size) {
                return filled;
            }
        }
    }
    return filled;
}


// Send data(Commands) to URG 
static int urg_sendTag(const char* tag) {

    char send_message[LineLength];
    sprintf(send_message, "%s\n", tag);
    int send_size = strlen(send_message);
    com_send(send_message, send_size);

    return send_size;
}


// Read data (Reply) from URG until the termination 
static int urg_readLine(char *buffer) {

    int i;
    for (i = 0; i < LineLength -1; ++i) {
        char recv_ch;
        int n = com_recv(&recv_ch, 1, Timeout);
        if (n <= 0) {
            if (i == 0) {
                return -1;		// timeout
            }
            break;
        }
        if ((recv_ch == '\r') || (recv_ch == '\n')) {
            break;
        }
        buffer[i] = recv_ch;
    }
    buffer[i] = '\0';

    return i;
}


// Send data (Commands) to URG and wait for reply
static int urg_sendMessage(const char* command, int timeout, int* recv_n) {

    int send_size = urg_sendTag(command);
    int recv_size = send_size + 2 + 1 + 2;
    char buffer[LineLength];

    int n = com_recv(buffer, recv_size, timeout);
    *recv_n = n;

    if (n < recv_size) {
        // When received size not matched
        return -1;
    }

    if (strncmp(buffer, command, send_size -1)) {
        // When command not matched
        return -1;
    }

    // !!! If possible do calculate check-sum to verify data

    // Convert the received data to Hex and return data.
    char reply_str[3] = "00";
    reply_str[0] = buffer[send_size];
    reply_str[1] = buffer[send_size + 1];
    return strtol(reply_str, NULL, 16);
}


// Read URG parameters
static int urg_getParameters(urg_state_t* state) {

    // Parameter read and prcessing (use)
    urg_sendTag("PP");
    char buffer[LineLength];
    int line_index = 0;
    enum {
        TagReply = 0,
        DataReply,
        Other,
    };
    int line_length;
    for (; (line_length = urg_readLine(buffer)) > 0; ++line_index) {

        if (line_index == Other + MODL) {
            buffer[line_length - 2] = '\0';
            state->model = &buffer[5];

        } else if (line_index == Other + DMIN) {
            state->distance_min = atoi(&buffer[5]);

        } else if (line_index == Other + DMAX) {
            state->distance_max = atoi(&buffer[5]);

        } else if (line_index == Other + ARES) {
            state->area_total = atoi(&buffer[5]);

        } else if (line_index == Other + AMIN) {
            state->area_min = atoi(&buffer[5]);
            state->first = state->area_min;

        } else if (line_index == Other + AMAX) {
            state->area_max = atoi(&buffer[5]);
            state->last = state->area_max;

        } else if (line_index == Other + AFRT) {
            state->area_front = atoi(&buffer[5]);

        } else if (line_index == Other + SCAN) {
            state->scan_rpm = atoi(&buffer[5]);
        }
    }

    if (line_index <= Other + SCAN) {
        return -1;
    }
    // Caluclate size of the data if necessary
    state->max_size = state->area_max +1;

    return 0;
}


// Process to connect with URG 
static int urg_connect(urg_state_t* state,
        const char* port, const long baudrate) {

    if (com_connect(port, baudrate) < 0) {
        ErrorMessage = "Cannot connect COM device.";
        return -1;
    }

    const long try_baudrate[] = { 19200, 115200, 38400 };
    size_t i;
    for (i = 0; i < sizeof(try_baudrate)/sizeof(try_baudrate[0]); ++i) {

        // Change baud rate to detect the compatible baud rate with sensor
        // !!! com_changeBaudrate(try_baudrate[i]);

        // Change to SCIP2.0 mode
        int recv_n = 0;
        urg_sendMessage("SCIP2.0", Timeout, &recv_n);
        if (recv_n <= 0) {
            // If there is no reply it is considered as baud rate incompatibility, try with different baud rate
            continue;
        }

        // Change the baud rate if not match the setting
        if (try_baudrate[i] != baudrate) {
            // !!! urg_changeBaudrate(baudrate);
            // !!! The change will be implemented after 1 scan.
            // !!! Thus, Host or PC should wait to send the command

            // !!! com_changeBaudrate(baudrate);
        }

        // Read parameter
        if (urg_getParameters(state) < 0) {
            ErrorMessage = "PP command fail.";
            return -1;
        }
        state->last_timestamp = 0;

        // URG is detected
        return 0;
    }

    // URG is not detected
    return -1;
}


// Disconnect URG 
static void urg_disconnect(void) {
    com_disconnect();
}


// Data read using GD-Command
static int urg_captureByGD(const urg_state_t* state) {

    char send_message[LineLength];
    sprintf(send_message, "GD%04d%04d%02d", state->first, state->last, 0);

    return urg_sendTag(send_message);
}


// Data read using MD-Command
static int urg_captureByMD(const urg_state_t* state, int capture_times) {

    char send_message[LineLength];
    sprintf(send_message, "MD%04d%04d%02d%01d%02d",
            state->first, state->last, 0, 0, capture_times);

    return urg_sendTag(send_message);
}


// Decode 6 bit data from URG 
static long urg_decode(const char* data, int data_byte) {
    long value = 0;
    int i;
    for (i = 0; i < data_byte; ++i) {
        value <<= 6;
        value &= ~0x3f;
        value |= data[i] - 0x30;
    }
    return value;
}


// Receive URG distance data 
static int urg_addRecvData(const char buffer[], long data[], int* filled) {

    static int remain_byte = 0;
    static char remain_data[3];
    const int data_byte = 3;

    const char* pre_p = buffer;
    const char* p = pre_p;

    if (remain_byte > 0) {
        memmove(&remain_data[remain_byte], buffer, data_byte - remain_byte);
        data[*filled] = urg_decode(remain_data, data_byte);
        ++(*filled);
        pre_p = &buffer[data_byte - remain_byte];
        p = pre_p;
        remain_byte = 0;
    }

    do {
        ++p;
        if ((p - pre_p) >= (int)(data_byte)) {
            data[*filled] = urg_decode(pre_p, data_byte);
            ++(*filled);
            pre_p = p;
        }
    } while (*p != '\0');
    remain_byte = p - pre_p;
    memmove(remain_data, pre_p, remain_byte);

    return 0;
}


// Receive URG data
static int urg_receiveData(urg_state_t* state, long data[], size_t max_size) {

    int filled = 0;

    // Fill the positions upto first or min by 19 (non-measurement range)
    int i;
    for (i = state->first -1; i >= 0; --i) {
        data[filled++] = 19;
    }

    char message_type = 'M';
    char buffer[LineLength];
    int line_length;
    for (i = 0; (line_length = urg_readLine(buffer)) >= 0; ++i) {

        // Verify the checksum
        if ((i >= 6) && (line_length == 0)) {

            // End of data receive
            size_t j;
            for (j = filled; j < max_size; ++j) {
                // Fill the position upto data end by 19 (non-measurement range)
                data[filled++] = 19;
            }
            return filled;

        } else if (i == 0) {
            // Judge the message (Command) by first letter of receive data
            if ((buffer[0] != 'M') && (buffer[0] != 'G')) {
                return -1;
            }
            message_type = buffer[0];

        } else if (! strncmp(buffer, "99b", 3)) {
            // Detect "99b" and assume [time-stamp] and [data] to follow
            i = 4;

        } else if ((i == 1) && (message_type == 'G')) {
            i = 4;

        } else if (i == 4) {
            // "99b" Fixed
            if (strncmp(buffer, "99b", 3)) {
                return -1;
            }

        } else if (i == 5) {
            state->last_timestamp = urg_decode(buffer, 4);

        } else if (i >= 6) {
            // Received Data
            if (line_length > (64 + 1)) {
                line_length = (64 + 1);
            }
            buffer[line_length -1] = '\0';
            int ret = urg_addRecvData(buffer, data, &filled);
            if (ret < 0) {
                return ret;
            }
        }
    }
    return -1;
}

