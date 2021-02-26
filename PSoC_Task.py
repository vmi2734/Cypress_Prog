import serial
ser = serial.Serial("COM4", 115200, timeout=1)
print("****************** "
           "PSoC 6 MCU: Work with LEDs, PWM, UART"
           "****************** \r\n")
def menu():
    print("****************** HELP MENU ***************** \r\n\n"
            "***** Please, enter one of these command:***** \r\n\n"
            "_________***********________\r\n\n "
            "1. *On LED3* - enable LED3;\r\n "
            "2. *On LED4* - enable LED4;\r\n "
            "3. *Off LED3* - disable LED3;\r\n "
            "4. *Off LED4* - disable LED4;\r\n "
            "5. *Blink LED3* - blink LED3;\r\n "
            "6. *Blink LED4* - blink LED4;\r\n "
            "________***********________\r\n\n ")
while(1):
    print ("\r\nCommand:\r\n")
    qt = input()

    if len(qt) > 20:
        print("The size of the command is incorrect ")
        menu()

    else:
        if qt == "exit":
            print(exit)
            exit()
                
        elif qt == ("On LED3") or qt == ("On LED4"):
            ser.write((qt+'\r').encode())
            print("\r\nDuty Cycle:\r\n")
            qt = input()
            ser.write((qt+'\r').encode())
            print("\nDuty Cycle = ", qt, " %")
            
        elif qt == ("Off LED3") or qt == ("Off LED4"):
            ser.write((qt+'\r').encode())
            print("\nLED Off")
            
        elif qt == ("Blink LED3") or qt == ("Blink LED4"):
            ser.write((qt+'\r').encode())
            print("\r\nDuty Cycle:\r\n")
            qt = input()
            ser.write((qt+'\r').encode())
            print("\nDuty Cycle = ", qt, " %")
            print("\r\nFrequency:\r\n")
            qt = input()
            ser.write((qt+'\r').encode())
            print("\nFreguency = ", qt, " Hz")
            print("\nLED Blink")
            
        else:
            menu()


