#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#include "my_secmalloc.private.h"

Test(logging, simple_logging, .init = cr_redirect_stderr)
{
    init_logging();
    log_general(1, LOG_INFO, "Hello, %s", "world");
    cr_expect_stderr_eq_str("Hello, world\n");
}

Test(mmap, simple_mmap)
{
    void *ptr = mmap(
        NULL,
        4096,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0);
    cr_expect(ptr != NULL);

    int res = munmap(ptr, 4096);
    cr_expect(res == 0);
}
