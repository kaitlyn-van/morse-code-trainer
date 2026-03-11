// PURPOSE: hold word lists for game
#ifndef WORD_BANK_H
#define WORD_BANK_H

// beginner words (3 letters)
const char* beginnerWords[] = {
    "SEA",
    "BEE",
    "FLY",
    "BUD",
    "CAM",
    "EWE",
    "FOG",
    "LOG",
    "HOG",
    "DOG",
    "SOS",
    "SUN",
    "MAP",
    "PEE",
    "CPR",
    "GPS",
    "AXE",
    "LOG",
    "SKY",
    "OWL",
    "BOW"
};

// store count of beginner words (allow for update if words are added/removed)
const int beginnerCount = sizeof(beginnerWords) / sizeof(beginnerWords[0]);

// medium words (4 letters)
const char* mediumWords[] = {
    "FIRE",
    "CAMP",
    "FOOD",
    "LIVE",
    "CHOP",
    "FISH",
    "TENT",
    "MOON",
    "MEAT",
    "HUNT",
    "GEAR",
    "HEAT",
    "SNOW",
    "RAIN"
};

const int mediumCount = sizeof(mediumWords) / sizeof(mediumWords[0]);

// hard words (5 letters)
const char* hardWords[] = {
    "WATER",
    "KNIFE",
    "BREAD",
    "SWORD",
    "STORM",
    "ARROW",
    "TORCH",
    "FIRES",
    "CAMPS",
    "TREAD"
};

const int hardCount = sizeof(hardWords) / sizeof(hardWords[0]);

#endif
