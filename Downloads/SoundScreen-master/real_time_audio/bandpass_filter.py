#!/usr/bin/python3

# http://indico.ictp.it/event/7657/session/4/contribution/19/material/1/0.pdf
# https://stackoverflow.com/questions/7543675/how-to-convert-pointer-to-c-array-to-python-array
# https://docs.python.org/3/c-api/intro.html
# https://docs.python.org/3/extending/embedding.html
# https://stackoverflow.com/questions/26384129/calling-c-from-python-passing-list-of-numpy-pointers

import bp_filter as bp

def bp_filter(data_C, cutoff_low, cutoff_high):
    
    test = bp.filter_audio(data_C, cutoff_low, cutoff_high)
    
    #f = open("py_out.txt", "w+")
    #for i in range(2048):
    #    f.write("%d\n" % test[i])
    #f.close()

    return test
    # print("bp_filter")  # testing

