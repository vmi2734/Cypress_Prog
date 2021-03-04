import serial
ser = serial.Serial("COM4", 115200, timeout=1)
print("****************** "
           "PSoC 6 MCU: Work with LEDs, PWM, UART"
           "****************** \r\n")
def menu():
    print("****************** HELP MENU ***************** \r\n\n"
            "***** Please, enter one of these command:***** \r\n\n"
            "________________***********________________\r\n\n "
            "- *On LED3,X* - enable LED3, X - Duty Cycle;\r\n "
            "- *On LED4,X* - enable LED4, X - Duty Cycle;\r\n "
            "- *Off LED3* - disable LED3;\r\n "
            "- *Off LED4* - disable LED4;\r\n "
            "- *Blink LED3,X,Y* - blink LED3, X-Duty Cycle,Y - Frequency;\r\n "
            "- *Blink LED4,X,Y* - blink LED4, X-Duty Cycle,Y - Frequency;\r\n "
            "- *Exit* - exit the programm.\r\n"
            "________________***********________________\r\n\n ")
    
def is_number(str):
    try:
        float(str)
        return True
    except ValueError:
        return False
    
def range_ds(str):
    try:
        val = float(str)
        if val >= 0.0 and val <= 100.0:
            return True
    except ValueError:
        return False

def range_fq(str):
    try:
        val = float(str)
        if val >= 0.0:
            return True
    except ValueError:
        return False
menu()    
while(1):
        
    print ("\r\nCommand:\r\n")
    qt = input()

    if len(qt) > 25:
        print("The size of the command is incorrect")
        menu()

    else:
        arr = qt.split(',')
        arr_len = len(arr)
        if arr[0] == "Off LED3" or arr[0] == "Off LED4":
            if arr_len == 1:
                ser.write((qt+'\r').encode())
            else:
                print("Bad command!")
                menu()
           
        elif arr[0] == "On LED3" or arr[0] == "On LED4":
            if arr_len != 2:
               print("No parameter *Duty Cycle*!") 
               menu()
            else:
                if is_number(arr[1]) == True and range_ds(arr[1]) == True:
                    ser.write((qt+'\r').encode())
                else:
                    print("Value of Duty Cycle is not correct!")
                    
        elif arr[0] == "Blink LED3" or arr[0] == "Blink LED4":
            if arr_len != 3:
                print("No parameters!")
                menu()
            else:
                if is_number(arr[1]) == True and range_ds(arr[1]) == True:
                    if is_number(arr[2]) == True and range_fq(arr[2]) == True:
                        ser.write((qt+'\r').encode())
                    else:
                        print("Value of Frequency is not correct!")
                else:
                    print("Value of Duty Cycle is not correct!")

        elif arr[0] == "Exit":
            if arr_len !=1:
                menu()
            else:
                print(exit)
                exit()
        else:
            menu()


