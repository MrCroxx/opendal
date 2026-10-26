#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "hf3fs_usrbio.h"

int main(int argc, char **argv) {
    assert(argc == 3);
    int fd = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, 0600);
    assert(fd >= 0);
    assert(hf3fs_is_hf3fs(fd));
    assert(hf3fs_reg_fd(fd, 0) <= 0);
    struct hf3fs_iov iov = {0};
    struct hf3fs_ior writer = {0}, reader = {0};
    assert(hf3fs_iovcreate(&iov, argv[1], 1 << 20, 0, -1) == 0);
    assert(hf3fs_iorcreate4(&writer, argv[1], 4, false, 0, 0, -1, 0) == 0);
    assert(hf3fs_iorcreate4(&reader, argv[1], 4, true, 0, 0, -1, 0) == 0);
    const size_t size = 131137;
    for (size_t i = 0; i < size; ++i) iov.base[i] = i % 251;
    assert(hf3fs_prep_io(&writer, &iov, false, iov.base, fd, 0, size, NULL) >= 0);
    assert(hf3fs_submit_ios(&writer) == 0);
    struct hf3fs_cqe cqe;
    assert(hf3fs_wait_for_ios(&writer, &cqe, 1, 1, NULL) == 1);
    assert(cqe.result == (int64_t)size);
    assert(fsync(fd) == 0);
    memset(iov.base, 0, size);
    assert(hf3fs_prep_io(&reader, &iov, true, iov.base, fd, 0, size, NULL) >= 0);
    assert(hf3fs_submit_ios(&reader) == 0);
    assert(hf3fs_wait_for_ios(&reader, &cqe, 1, 1, NULL) == 1);
    assert(cqe.result == (int64_t)size);
    for (size_t i = 0; i < size; ++i) assert(iov.base[i] == i % 251);
    memset(iov.base, 0, size);
    assert(hf3fs_prep_io(&reader, &iov, true, iov.base, fd, 17, 65537, NULL) >= 0);
    assert(hf3fs_submit_ios(&reader) == 0);
    assert(hf3fs_wait_for_ios(&reader, &cqe, 1, 1, NULL) == 1);
    assert(cqe.result == 65537);
    for (size_t i = 0; i < 65537; ++i) assert(iov.base[i] == (i + 17) % 251);
    hf3fs_iordestroy(&reader);
    hf3fs_iordestroy(&writer);
    hf3fs_iovdestroy(&iov);
    hf3fs_dereg_fd(fd);
    assert(close(fd) == 0);
    assert(unlink(argv[2]) == 0);
    puts("USRBIO write, read, and unaligned range read passed");
}
