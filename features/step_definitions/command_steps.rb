When(/^I run `([^`]*)` on cpu ([\d]+)$/) do |cmd, cpu|
  cmd = sanitize_text(cmd)
  cmd = "taskset -c #{cpu} #{cmd}"
  run_command_and_stop(cmd, fail_on_error: false)
end

When(/^I run `([^`]*)` in background on cpu ([\d]+)$/) do |cmd, cpu|
  cmd = sanitize_text(cmd)
  cmd = "taskset -c #{cpu} #{cmd}"
  run_command(cmd)
end

When(/^I run `([^`]*)` on cpu ([\d]+) in background$/) do |cmd, cpu|
  cmd = sanitize_text(cmd)
  cmd = "taskset -c #{cpu} #{cmd}"
  run_command(cmd)
end
