#include <stdio.h>
#include <stdlib.h>

// takes in a string input and parses it into a standard file path string
// TO DO: add error catcher for invalid file paths
// handle going to parent and own directory in actual reader code


// removes repeated forward slashes
const char * cleanInput(char * input){
    int i = 0;
    char * output = (char*) malloc((4096)*sizeof(char));

    if(*input != '/'){
        return -1;
    } 

    do{
        output[i++] = '/';
        
        // ignore repeated slashes
        while(*input != '\0' &&  *input == '/'){
            input++;
        }

        char curr[256];
        int j = 0;

        while(*input != '\0' && *input != '/'){
            output[i++] = *input;
            input++;
        }

    }while(*(input++) != '\0');

    return output;
}