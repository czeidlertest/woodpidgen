#ifndef ERROR_CODES_H
#define ERROR_CODES_H

namespace WP {
enum err {
    kUnkown = -50000,
    kUninit,
    kNoKeyStore,
    kEntryNotFound,

    kError = -1,
    kOk = 0,
};
}

#endif // ERROR_CODES_H
