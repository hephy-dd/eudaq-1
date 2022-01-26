#include "DBRInfoStream.h"
#include "cadef.h"

#include <iomanip>

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_char &rCtrl) noexcept {
  rStream << "op. status: " << ca_message(rCtrl.status) << '\n'
          << "alarm: " << rCtrl.severity << '\n'
          << "units: " << rCtrl.units << '\n'
          << "upper display limit: " << rCtrl.upper_disp_limit << '\n'
          << "lower display limit: " << rCtrl.lower_disp_limit << '\n'
          << "upper alarm limit: " << rCtrl.upper_alarm_limit << '\n'
          << "lower alarm limit: " << rCtrl.lower_alarm_limit << '\n'
          << "upper warning limit: " << rCtrl.upper_warning_limit << '\n'
          << "lower warning limit: " << rCtrl.lower_warning_limit << '\n'
          << "upper ctrl limit: " << rCtrl.upper_ctrl_limit << '\n'
          << "lower ctrl limit: " << rCtrl.lower_ctrl_limit << '\n';
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_short &rCtrl) noexcept {
  rStream << "op. status: " << ca_message(rCtrl.status) << '\n'
          << "alarm: " << rCtrl.severity << '\n'
          << "units: " << rCtrl.units << '\n'
          << "upper display limit: " << rCtrl.upper_disp_limit << '\n'
          << "lower display limit: " << rCtrl.lower_disp_limit << '\n'
          << "upper alarm limit: " << rCtrl.upper_alarm_limit << '\n'
          << "lower alarm limit: " << rCtrl.lower_alarm_limit << '\n'
          << "upper warning limit: " << rCtrl.upper_warning_limit << '\n'
          << "lower warning limit: " << rCtrl.lower_warning_limit << '\n'
          << "upper ctrl limit: " << rCtrl.upper_ctrl_limit << '\n'
          << "lower ctrl limit: " << rCtrl.lower_ctrl_limit << '\n';
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_long &rCtrl) noexcept {
  rStream << "op. status: " << ca_message(rCtrl.status) << '\n'
          << "alarm: " << rCtrl.severity << '\n'
          << "units: " << rCtrl.units << '\n'
          << "upper display limit: " << rCtrl.upper_disp_limit << '\n'
          << "lower display limit: " << rCtrl.lower_disp_limit << '\n'
          << "upper alarm limit: " << rCtrl.upper_alarm_limit << '\n'
          << "lower alarm limit: " << rCtrl.lower_alarm_limit << '\n'
          << "upper warning limit: " << rCtrl.upper_warning_limit << '\n'
          << "lower warning limit: " << rCtrl.lower_warning_limit << '\n'
          << "upper ctrl limit: " << rCtrl.upper_ctrl_limit << '\n'
          << "lower ctrl limit: " << rCtrl.lower_ctrl_limit << '\n';
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_enum &rCtrl) noexcept {
  rStream << "op. status: " << ca_message(rCtrl.status) << '\n'
          << "alarm: " << rCtrl.severity << '\n'
          << "def. states: " << rCtrl.no_str << '\n';
  for (auto i = 0; i < rCtrl.no_str; ++i)
    rStream << "  " << std::setw(2) << std::setfill('0') << i << std::setw(9)
            << std::setfill('.') << ' ' << ' ' << rCtrl.strs[i] << '\n';
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream,
                         const dbr_ctrl_float &rCtrl) noexcept {
  rStream << "op. status: " << ca_message(rCtrl.status) << '\n'
          << "alarm: " << rCtrl.severity << '\n'
          << "units: " << rCtrl.units << '\n'
          << "precision: " << rCtrl.precision << '\n'
          << "upper display limit: " << rCtrl.upper_disp_limit << '\n'
          << "lower display limit: " << rCtrl.lower_disp_limit << '\n'
          << "upper alarm limit: " << rCtrl.upper_alarm_limit << '\n'
          << "lower alarm limit: " << rCtrl.lower_alarm_limit << '\n'
          << "upper warning limit: " << rCtrl.upper_warning_limit << '\n'
          << "lower warning limit: " << rCtrl.lower_warning_limit << '\n'
          << "upper ctrl limit: " << rCtrl.upper_ctrl_limit << '\n'
          << "lower ctrl limit: " << rCtrl.lower_ctrl_limit << '\n';
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream,
                          const dbr_ctrl_double &rCtrl) noexcept {
   rStream << "op. status: " << ca_message(rCtrl.status) << '\n'
           << "alarm: " << rCtrl.severity << '\n'
           << "units: " << rCtrl.units << '\n'
           << "precision: " << rCtrl.precision << '\n'
           << "upper display limit: " << rCtrl.upper_disp_limit << '\n'
           << "lower display limit: " << rCtrl.lower_disp_limit << '\n'
           << "upper alarm limit: " << rCtrl.upper_alarm_limit << '\n'
           << "lower alarm limit: " << rCtrl.lower_alarm_limit << '\n'
           << "upper warning limit: " << rCtrl.upper_warning_limit << '\n'
           << "lower warning limit: " << rCtrl.lower_warning_limit << '\n'
           << "upper ctrl limit: " << rCtrl.upper_ctrl_limit << '\n'
           << "lower ctrl limit: " << rCtrl.lower_ctrl_limit << '\n';
   return rStream;
 }
