
#include <M5StickC.h>
#include <math.h>

HardwareSerial VSerial(1);
TFT_eSprite tft = TFT_eSprite(&M5.Lcd);

uint8_t I2CWrite1Byte(uint8_t Addr, uint8_t Data)
{
    Wire.beginTransmission(0x38);
    Wire.write(Addr);
    Wire.write(Data);
    return Wire.endTransmission();
}

uint8_t I2CWritebuff(uint8_t Addr, uint8_t *Data, uint16_t Length)
{
    Wire.beginTransmission(0x38);
    Wire.write(Addr);
    for (int i = 0; i < Length; i++)
    {
        Wire.write(Data[i]);
    }
    return Wire.endTransmission();
}

uint8_t Setspeed(int16_t Vtx, int16_t Vty, int16_t Wt)
{
    int16_t speed_buff[4] = {0};
    int8_t speed_sendbuff[4] = {0};

    Wt = (Wt > 100) ? 100 : Wt;
    Wt = (Wt < -100) ? -100 : Wt;

    Vtx = (Vtx > 100) ? 100 : Vtx;
    Vtx = (Vtx < -100) ? -100 : Vtx;
    Vty = (Vty > 100) ? 100 : Vty;
    Vty = (Vty < -100) ? -100 : Vty;

    Vtx = (Wt != 0) ? Vtx * (100 - abs(Wt)) / 100 : Vtx;
    Vty = (Wt != 0) ? Vty * (100 - abs(Wt)) / 100 : Vty;

    speed_buff[0] = Vty - Vtx - Wt;
    speed_buff[1] = Vty + Vtx + Wt;
    speed_buff[3] = Vty - Vtx + Wt;
    speed_buff[2] = Vty + Vtx - Wt;

    for (int i = 0; i < 4; i++)
    {
        speed_buff[i] = (speed_buff[i] > 100) ? 100 : speed_buff[i];
        speed_buff[i] = (speed_buff[i] < -100) ? -100 : speed_buff[i];
        speed_sendbuff[i] = speed_buff[i];
    }
    return I2CWritebuff(0x00, (uint8_t *)speed_sendbuff, 4);
}

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(0);

    tft.setColorDepth(8);
    tft.createSprite(80, 160);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);

    VSerial.begin(115200, SERIAL_8N1, 33, 32);
    Wire.begin(0, 26);

    Setspeed(0, 0, 0);

    VSerial.write(0xAF);
    tft.fillSprite(TFT_ORANGE);
    tft.pushSprite(0,0);
}

int16_t ux;
unsigned long T;
#define BASE_SPEED  20
bool last_dir = false;
void loop()
{
    M5.update();
    if(M5.BtnA.wasReleased())
    {
        Setspeed(0, 0, 0);
        ESP.restart();
    }

    if(VSerial.available())
    {
        VSerial.write(0xAF);

        uint8_t b_data[4];
        VSerial.readBytes(b_data, 4);

        int8_t ux = b_data[0];
        
        uint32_t area = b_data[1] << 16 | b_data[2] << 8 | b_data[3];
        int8_t uy;
        Serial.printf("%d, %d\n", ux, area);
        if(area < 500)
        {
            if(last_dir)
                Setspeed(0, 0, 15);
            else
                Setspeed(0, 0, -15);
            tft.fillSprite(TFT_RED);
            tft.pushSprite(0,0);
            return;
        }
        else
        {
            tft.fillSprite(TFT_GREEN);
            tft.pushSprite(0,0);
            if (area < 20000)
            {
                uy = 1 / (0.00001 * area) + 15;
                if(uy > 40)
                    uy = 40;
                if(uy < 5)
                    uy = 5;
                Setspeed(ux, uy, 0);
            }
            else if (area > 24000)
            {
                Setspeed(ux, -10, 0);
            }
            else
            {
                Setspeed(ux, 0, 0);
            }
        }

        if(ux > 0)
        {
            last_dir = true;
        }
        else
        {
            last_dir = false;
        }
    }
}