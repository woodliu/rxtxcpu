Given(/^I wait ([\d]+.[\d]+) seconds? for (?:a|the) command to start up$/) do |seconds|
  aruba.config.startup_wait_time = seconds.to_f
end
