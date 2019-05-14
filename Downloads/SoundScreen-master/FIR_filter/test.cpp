#include "jfastfir.h"

int main ()
{
    //create pointer
    JFastFIRFilter *fastfir;

    //create FastFIR filter
    fastfir = new JFastFIRFilter;

    //set the kernel of the filter, in this case a Low Pass filter at 800Hz
    fastfir->setKernel(JFilterDesign::LowPassHanning(800,48000,1001));

    //process data (data is vector<kffsamp_t>, eg "vector<kffsamp_t> data;")
    fastfir->Update(data);

    //delete FastFIR filter
    delete fastfir;
}