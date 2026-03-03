// PURPOSE: This program is a Morse code lookup table along with functions to set up the morse code trainer game

#include <string.h>
#include "morse_lookup.h" 

// structure to hold the letter and its corresponding morse code
struct MorseEntry {
    char letter; // the letter
    const char* morseCode; // the morse code representation of the letter
};

// morse code lookup table
const MorseEntry morseTable[] = {
    {'A', ".-"},
    {'B', "-..."},
    {'C', "-.-."},
    {'D', "-.."},
    {'E', "."},
    {'F', "..-."},
    {'G', "--."},
    {'H', "...."},
    {'I', ".."},
    {'J', ".---"},
    {'K', "-.-"},
    {'L', ".-.."},
    {'M', "--"},
    {'N', "-."},
    {'O', "---"},
    {'P', ".--."},
    {'Q', "--.-"},
    {'R', ".-."},
    {'S', "..."},
    {'T', "-"},
    {'U', "..-"},
    {'V', "...-"},
    {'W', ".--"},
    {'X', "-..-"},
    {'Y', "-.--"},
    {'Z', "--.."}
};

// number of entries in table
const size_t n_entries = sizeof(morseTable) / sizeof(morseTable[0]);

// decode function using lookup table
char decodeMorse(const char* symbols){

    // cleaning any potential empty symbols
    if(symbols == nullptr){
        return '?';
    }
    if(symbols[0] == '\0'){
        return '?';
    }
    // loop through each entry
    for(int i = 0; i < n_entries; i++){
        int compare = strcmp(symbols, morseTable[i].morseCode);
        if(compare == 0){
            return morseTable[i].letter;
        };
    }
    return '?';
}
