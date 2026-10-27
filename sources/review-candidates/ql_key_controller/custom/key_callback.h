#ifndef KEY_CALLBACK_H
#define KEY_CALLBACK_H

#define ARRAY_SIZE(array)   (sizeof(array)/sizeof(array[0]))

typedef void (*keycallback)(void*);

typedef enum {
	PRESS_DOWN = 0,
	PRESS_UP = 1,
    LONG_PRESS = 2,
	NUM_OF_EVENT,
	NONE_PRESS
}key_state;

typedef struct 
{
    const char *name;
    keycallback cb[NUM_OF_EVENT];
} key_handler_data_t;

#endif