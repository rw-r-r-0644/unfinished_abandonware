#include <string.h>
#include <coreinit/title.h>
#include "common/common.h"
#include "utils/utils.h"
#include "main.h"

static volatile uint8_t ucRunOnce = 0;

int main(int argc, char **argv)
{
    //! *******************************************************************
    //! *                 Jump to our application                    *
    //! *******************************************************************
    return Menu_Main();
}
