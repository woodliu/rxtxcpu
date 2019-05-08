# `chcpu` is not available in trusty. `/sys/devices/system/cpu/cpu*/online` is
# not available on ubuntu. `/sys/devices/system/node/node*/cpu*/online` is
# available in all our current test environments. We'll use that.

Given /^I disable cpu ([\d]+)$/ do |cpu|
  cmd = %Q{
    sudo bash -c '
      (echo 0 > /sys/devices/system/node/node[0-9]*/cpu#{cpu}/online) 2>/dev/null
      grep "^0\$" /sys/devices/system/node/node[0-9]*/cpu#{cpu}/online
    '
  }
  run_command_and_stop(sanitize_text(cmd), fail_on_error: true)
end

Given /^I enable cpu ([\d]+)$/ do |cpu|
  cmd = %Q{
    sudo bash -c '
      (echo 1 > /sys/devices/system/node/node[0-9]*/cpu#{cpu}/online) 2>/dev/null
      grep "^1\$" /sys/devices/system/node/node[0-9]*/cpu#{cpu}/online
    '
  }
  run_command_and_stop(sanitize_text(cmd), fail_on_error: true)
end
