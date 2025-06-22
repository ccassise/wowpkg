#include <assert.h>

#include "osstring.h"

static void test_os_strcasestr(void)
{
    const char *input = "n e e d l e HAYSTACKHA HAY HAYSTACK NEEEDLEHACKSTACK  needle";
    assert(os_strcasestr(input, "NEEDLE") != NULL);

    input = "n e e d l e HAYSTACKHA HAY HAYSTACK NEEEDLEHACKSTACK";
    assert(os_strcasestr(input, "nEEdlE") == NULL);

    input = "NEEdleHAYhaysTACKhaystackneed";
    assert(os_strcasestr(input, "needle") != NULL);

    input = "haystack needle haystack";
    assert(os_strcasestr(input, "needle") != NULL);

    input = "\nneedle\n";
    assert(os_strcasestr(input, "Needle") != NULL);

    input = "needle";
    assert(os_strcasestr(input, "Needle") != NULL);

    input = "needl";
    assert(os_strcasestr(input, "needle") == NULL);

    input = "eedle";
    assert(os_strcasestr(input, "needle") == NULL);
}

int main(void)
{
    test_os_strcasestr();
    return 0;
}
