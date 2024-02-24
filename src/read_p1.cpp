#include "read_p1.h"

unsigned int currentCRC = 0;
struct TelegramDecodedObject telegramObjects[NUMBER_OF_READOUTS];
char telegram[P1_MAXLINELENGTH];

unsigned int crc16(unsigned int crc, unsigned char *buf, int len)
{
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned int)buf[pos];

        for (int i = 8; i != 0; i--)
        {
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool isNumber(char *res, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0))
        {
            return false;
        }
    }
    return true;
}

int findCharInArrayRev(char array[], char c, int len)
{
    for (int i = len - 1; i >= 0; i--)
    {
        if (array[i] == c)
        {
            return i;
        }
    }

    return -1;
}

long getValue(char *buffer, int maxlen, char startchar, char endchar)
{
    int s = findCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = findCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l))
    {
        if (endchar == '*')
        {
            if (isNumber(res, l))
                return (1000 * atof(res));
        }
        else if (endchar == ')')
        {
            if (isNumber(res, l))
                return atof(res);
        }
    }

    return 0;
}

/**
 *  Decodes the telegram PER line. Not the complete message. 
 */
bool decodeTelegram(int len)
{
    int startChar = findCharInArrayRev(telegram, '/', len);
    int endChar = findCharInArrayRev(telegram, '!', len);
    bool validCRCFound = false;

#ifdef DEBUG
    for (int cnt = 0; cnt < len; cnt++)
    {
        Serial.print(telegram[cnt]);
    }
    Serial.print("\n");
#endif

    if (startChar >= 0)
    {
        // * Start found. Reset CRC calculation
        currentCRC = crc16(0x0000, (unsigned char *)telegram + startChar, len - startChar);
    }
    else if (endChar >= 0)
    {
        // * Add to crc calc
        currentCRC = crc16(currentCRC, (unsigned char *)telegram + endChar, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + endChar + 1, 4);

        messageCRC[4] = 0; // * Thanks to HarmOtten (issue 5)
        validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);

#ifdef DEBUG
        if (validCRCFound)
            Serial.println(F("CRC Valid!"));
        else
            Serial.println(F("CRC Invalid!"));
#endif
        currentCRC = 0;
    }
    else
    {
        currentCRC = crc16(currentCRC, (unsigned char *)telegram, len);
    }

    // Loops throug all the telegramObjects to find the code in the telegram line
    // If it finds the code the value will be stored in the object so it can later be send to the mqtt broker
    for (int i = 0; i < NUMBER_OF_READOUTS; i++)
    {
        if (strncmp(telegram, telegramObjects[i].code, strlen(telegramObjects[i].code)) == 0)
        {
            long newValue = getValue(telegram, len, telegramObjects[i].startChar, telegramObjects[i].endChar);
            if (newValue != telegramObjects[i].value)
            {
                telegramObjects[i].value = newValue;
            }
            break;

#ifdef DEBUG
            Serial.println((String) "Found a Telegram object: " + telegramObjects[i].name + " value: " + telegramObjects[i].value);
#endif
        }
    }

    return validCRCFound;
}

bool readP1Serial()
{
    if (Serial2.available())
    {
#ifdef DEBUG
        Serial.println("Serial2 is available");
        Serial.println("Memset telegram");
#endif
        memset(telegram, 0, sizeof(telegram));
        while (Serial2.available())
        {
            // Reads the telegram untill it finds a return character
            // That is after each line in the telegram
            int len = Serial2.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);

            telegram[len] = '\n';
            telegram[len + 1] = 0;

            bool result = decodeTelegram(len + 1);
            // When the CRC is check which is also the end of the telegram
            // if valid decode return true
            if (result)
            {
                return true;
            }
        }
    }
    return false;
}

void setupDataReadout(void)
{
    // 1-0:1.8.1(000992.992*kWh)
    // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v5.0)
    telegramObjects[0].name = "consumption_tarif_1";
    strcpy(telegramObjects[0].code, "1-0:1.8.1");
    telegramObjects[0].endChar = '*';

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v5.0)
    telegramObjects[1].name = "consumption_tarif_2";
    strcpy(telegramObjects[1].code, "1-0:1.8.2");
    telegramObjects[1].endChar = '*';

    telegramObjects[0].name = "received_tarif_1";
    strcpy(telegramObjects[0].code, "1-0:2.8.1");
    telegramObjects[0].endChar = '*';

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v5.0)
    telegramObjects[1].name = "received_tarif_2";
    strcpy(telegramObjects[1].code, "1-0:2.8.2");
    telegramObjects[1].endChar = '*';

    // 1-0:1.7.0(00.424*kW) Actueel verbruik
    // 1-0:1.7.x = Electricity consumption actual usage (DSMR v5.0)
    telegramObjects[2].name = "actual_consumption";
    strcpy(telegramObjects[2].code, "1-0:1.7.0");
    telegramObjects[2].endChar = '*';

    // 1-0:2.7.0(00.000*kW) Actuele teruglevering (-P) in 1 Watt resolution
    telegramObjects[3].name = "actual_received";
    strcpy(telegramObjects[3].code, "1-0:2.7.0");
    telegramObjects[3].endChar = '*';

    // 1-0:21.7.0(00.378*kW)
    // 1-0:21.7.0 = Instantaan vermogen Elektriciteit levering L1
    telegramObjects[4].name = "instant_power_usage_l1";
    strcpy(telegramObjects[4].code, "1-0:21.7.0");
    telegramObjects[4].endChar = '*';

    // 1-0:41.7.0(00.378*kW)
    // 1-0:41.7.0 = Instantaan vermogen Elektriciteit levering L2
    telegramObjects[5].name = "instant_power_usage_l2";
    strcpy(telegramObjects[5].code, "1-0:41.7.0");
    telegramObjects[5].endChar = '*';

    // 1-0:61.7.0(00.378*kW)
    // 1-0:61.7.0 = Instantaan vermogen Elektriciteit levering L3
    telegramObjects[6].name = "instant_power_usage_l3";
    strcpy(telegramObjects[6].code, "1-0:61.7.0");
    telegramObjects[6].endChar = '*';

    // 1-0:22.7.0(00.378*kW)
    // 1-0:22.7.0 = Instantaan vermogen Elektriciteit teruglevering L1
    telegramObjects[4].name = "instant_power_return_l1";
    strcpy(telegramObjects[4].code, "1-0:22.7.0");
    telegramObjects[4].endChar = '*';

    // 1-0:42.7.0(00.378*kW)
    // 1-0:42.7.0 = Instantaan vermogen Elektriciteit teruglevering L2
    telegramObjects[5].name = "instant_power_return_l2";
    strcpy(telegramObjects[5].code, "1-0:42.7.0");
    telegramObjects[5].endChar = '*';

    // 1-0:62.7.0(00.378*kW)
    // 1-0:62.7.0 = Instantaan vermogen Elektriciteit teruglevering L3
    telegramObjects[6].name = "instant_power_return_l3";
    strcpy(telegramObjects[6].code, "1-0:62.7.0");
    telegramObjects[6].endChar = '*';

    // 1-0:31.7.0(002*A)
    // 1-0:31.7.0 = Instantane stroom Elektriciteit L1
    telegramObjects[7].name = "instant_power_current_l1";
    strcpy(telegramObjects[7].code, "1-0:31.7.0");
    telegramObjects[7].endChar = '*';

    // 1-0:51.7.0(002*A)
    // 1-0:51.7.0 = Instantane stroom Elektriciteit L2
    telegramObjects[8].name = "instant_power_current_l2";
    strcpy(telegramObjects[8].code, "1-0:51.7.0");
    telegramObjects[8].endChar = '*';

    // 1-0:71.7.0(002*A)
    // 1-0:71.7.0 = Instantane stroom Elektriciteit L3
    telegramObjects[9].name = "instant_power_current_l3";
    strcpy(telegramObjects[9].code, "1-0:71.7.0");
    telegramObjects[9].endChar = '*';

    // 1-0:32.7.0(232.0*V)
    // 1-0:32.7.0 = Voltage L1
    telegramObjects[10].name = "instant_voltage_l1";
    strcpy(telegramObjects[10].code, "1-0:32.7.0");
    telegramObjects[10].endChar = '*';

    // 1-0:52.7.0(232.0*V)
    // 1-0:52.7.0 = Voltage L2
    telegramObjects[11].name = "instant_voltage_l2";
    strcpy(telegramObjects[11].code, "1-0:52.7.0");
    telegramObjects[11].endChar = '*';

    // 1-0:72.7.0(232.0*V)
    // 1-0:72.7.0 = Voltage L3
    telegramObjects[12].name = "instant_voltage_l3";
    strcpy(telegramObjects[12].code, "1-0:72.7.0");
    telegramObjects[12].endChar = '*';

    // 0-0:96.14.0(0001)
    // 0-0:96.14.0 = Actual Tarif
    telegramObjects[13].name = "actual_tarif_group";
    strcpy(telegramObjects[13].code, "0-0:96.14.0");

    // 0-1:24.2.3(150531200000S)(00811.923*m3)
    // 0-1:24.2.3 = Gas (DSMR v5.0) on Belgian meters
    telegramObjects[18].name = "gas_meter_m3";
    strcpy(telegramObjects[18].code, "0-1:24.2.3");
    telegramObjects[12].endChar = '*';

#ifdef DEBUG
    Serial.println("Telegram data initialized:");
    for (int i = 0; i < NUMBER_OF_READOUTS; i++)
        Serial.println(telegramObjects[i].name);
#endif
}