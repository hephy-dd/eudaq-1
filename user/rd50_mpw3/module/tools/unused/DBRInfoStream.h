#ifndef DBRINFOSTREAM_H
#define DBRINFOSTREAM_H

#include <ostream>

struct dbr_ctrl_char;
struct dbr_ctrl_long;
struct dbr_ctrl_short;
struct dbr_ctrl_enum;
struct dbr_ctrl_float;
struct dbr_ctrl_double;

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_char &rCtrl) noexcept;

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_short &rCtrl) noexcept;

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_long &rCtrl) noexcept;

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_enum &rCtrl) noexcept;

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_float &rCtrl) noexcept;

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_double &rCtrl) noexcept;

#endif // STREAMDBR_CTRL_H
