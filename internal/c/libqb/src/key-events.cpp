#include "key-events.h"

#include <cstdlib>

onkey_struct *onkey = (onkey_struct *)calloc(32, sizeof(onkey_struct));
int32_t onkey_inprogress = 0;
