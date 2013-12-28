###########
# Generate Unique Strings
# - Store them as a pickle file
###########

# References:
# http://stackoverflow.com/questions/2257441/python-random-string-generation-with-upper-case-letters-and-digits

import string
import random
import pickle

setSize = 500
uniqueLen = 4

uniqueSet = []

print "Generating Set"
while (len(uniqueSet) < setSize):
    if len(uniqueSet)%50 == 49:
        print str(len(uniqueSet)+1) + " values have been generated"
    n = random.choice(range(0xFFFF))
    if n not in uniqueSet:
        uniqueSet.append(n)
        
pickle.dump(uniqueSet, open( "uniqueSet.pkl", "wb"))

print "Saved to uniqueSet.pkl"
print



