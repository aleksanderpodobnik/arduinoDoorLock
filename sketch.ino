#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Servo.h>

LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // RS=7, E=6, D4=5, D5=4, D6=3, D7=2

constexpr uint8_t SS_PIN = 10; // RC522 SDA
constexpr uint8_t RST_PIN = 9; // RC522 RST
MFRC522 mfrc522(SS_PIN, RST_PIN);

constexpr uint8_t GREEN_LED = 8;
constexpr uint8_t RED_LED = A0;

constexpr uint8_t SERVO_PIN = A1; // PWM wire is on A1
Servo door;

const byte ACCEPT_UID[] = {0xAB, 0xAB, 0xAB, 0xAB};
const byte DENY_UID[] = {0x10, 0x10, 0x10, 0x10};
const byte ACCEPT_UID_LEN = sizeof(ACCEPT_UID);
const byte DENY_UID_LEN = sizeof(DENY_UID);

bool uidEquals(const MFRC522::Uid &uid, const byte *ref, byte refLen)
{
    if (uid.size != refLen)
        return false;
    for (byte i = 0; i < refLen; i++)
        if (uid.uidByte[i] != ref[i])
            return false;
    return true;
}
String uidToString(const MFRC522::Uid &uid)
{
    String s;
    for (byte i = 0; i < uid.size; i++)
    {
        if (i)
            s += ":";
        if (uid.uidByte[i] < 0x10)
            s += "0";
        s += String(uid.uidByte[i], HEX);
    }
    s.toUpperCase();
    return s;
}
void blink(uint8_t pin, uint8_t times, uint16_t on_ms = 120, uint16_t off_ms = 120)
{
    for (uint8_t i = 0; i < times; i++)
    {
        digitalWrite(pin, HIGH);
        delay(on_ms);
        digitalWrite(pin, LOW);
        delay(off_ms);
    }
}
void lcdMsg(const char *line1, const char *line2 = "")
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void setup()
{
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);

    delay(100);
    lcd.begin(16, 2);
    lcdMsg("RFID Ready", "Scan a card");

    door.attach(SERVO_PIN);
    door.write(0); // set your closed angle here (0–180)
    delay(300);

    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    SPI.begin();
    mfrc522.PCD_Init();
    delay(50);
    Serial.println("RC522 initialized. Present a card...");
}

void openDoor(uint16_t hold_ms = 2000)
{
    door.write(90); // open position (adjust as needed)
    delay(hold_ms);
    door.write(0); // back to closed
    delay(300);
}

void loop()
{
    if (!mfrc522.PICC_IsNewCardPresent())
        return;
    if (!mfrc522.PICC_ReadCardSerial())
        return;

    String uidStr = uidToString(mfrc522.uid);
    Serial.print("UID: ");
    Serial.println(uidStr);

    bool isAccepted = uidEquals(mfrc522.uid, ACCEPT_UID, ACCEPT_UID_LEN);
    bool isDenied = uidEquals(mfrc522.uid, DENY_UID, DENY_UID_LEN);

    if (isAccepted)
    {
        lcdMsg("ACCESS GRANTED", uidStr.c_str());
        blink(GREEN_LED, 5);
        openDoor(); // move the servo
    }
    else if (isDenied)
    {
        lcdMsg("ACCESS DENIED", uidStr.c_str());
        blink(RED_LED, 5);
    }
    else
    {
        lcdMsg("UNKNOWN CARD", uidStr.c_str());
        blink(RED_LED, 5, 80, 80);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    delay(300);
    while (mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial())
    {
        delay(50);
    }
    lcdMsg("RFID Ready", "Scan a card");
}