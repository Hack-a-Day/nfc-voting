###########
# Generate UID Test in C Header File
# 
###########

import string
import random
import pickle

setSize = 500
uniqueLen = 7

uniqueSet = []

print "Generating Set"
while (len(uniqueSet) < setSize):
    if len(uniqueSet)%50 == 49:
        print str(len(uniqueSet)+1) + " values have been generated"
    thisUID = []
    for i in range(7):
        thisUID.append(random.choice(range(0xFF)))
    if thisUID not in uniqueSet:
        print thisUID
        uniqueSet.append(thisUID)
        
print "Generating PROGMEM array as uniqueSet.h"
with open('uniqueSet.h', 'w') as f:
    f.write("//THIS IS FOR TESTING ONLY!!!!!!\n\n")
    f.write("unsigned int uniqueSetLen = " + str(setSize) + ";\n\n")
    f.write("unsigned char uniqueSet[" + str(setSize)*uniqueLen + "] PROGMEM = {\n")
    for i in range(len(uniqueSet)):
        outStr = "  "
        for d in range(6):
            outStr += "0x{:02x}".format(uniqueSet[i][d]).upper().replace("X","x") + ", "
            
        outStr += "0x{:02x}".format(uniqueSet[i][6]).upper().replace("X","x")
        if (i == (len(uniqueSet)-1)):
            outStr += "\n"
        else:
            outStr += ",\n"
        f.write(outStr)
    f.write("};\n")    

