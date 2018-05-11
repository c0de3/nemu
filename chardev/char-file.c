/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/option.h"
#include "chardev/char.h"

#include "chardev/char-fd.h"

static void qmp_chardev_open_file(Chardev *chr,
                                  ChardevBackend *backend,
                                  bool *be_opened,
                                  Error **errp)
{
    ChardevFile *file = backend->u.file.data;
    int flags, in = -1, out;

    flags = O_WRONLY | O_CREAT | O_BINARY;
    if (file->has_append && file->append) {
        flags |= O_APPEND;
    } else {
        flags |= O_TRUNC;
    }

    out = qmp_chardev_open_file_source(file->out, flags, errp);
    if (out < 0) {
        return;
    }

    if (file->has_in) {
        flags = O_RDONLY;
        in = qmp_chardev_open_file_source(file->in, flags, errp);
        if (in < 0) {
            qemu_close(out);
            return;
        }
    }

    qemu_chr_open_fd(chr, in, out);
}

static void qemu_chr_parse_file_out(QemuOpts *opts, ChardevBackend *backend,
                                    Error **errp)
{
    const char *path = qemu_opt_get(opts, "path");
    ChardevFile *file;

    backend->type = CHARDEV_BACKEND_KIND_FILE;
    if (path == NULL) {
        error_setg(errp, "chardev: file: no filename given");
        return;
    }
    file = backend->u.file.data = g_new0(ChardevFile, 1);
    qemu_chr_parse_common(opts, qapi_ChardevFile_base(file));
    file->out = g_strdup(path);

    file->has_append = true;
    file->append = qemu_opt_get_bool(opts, "append", false);
}

static void char_file_class_init(ObjectClass *oc, void *data)
{
    ChardevClass *cc = CHARDEV_CLASS(oc);

    cc->parse = qemu_chr_parse_file_out;
    cc->open = qmp_chardev_open_file;
}

static const TypeInfo char_file_type_info = {
    .name = TYPE_CHARDEV_FILE,
    .parent = TYPE_CHARDEV_FD,
    .class_init = char_file_class_init,
};

static void register_types(void)
{
    type_register_static(&char_file_type_info);
}

type_init(register_types);
