Given /^I mark the kernel ring buffer with "(.*?)"$/ do |mark|
  cmd = %Q{sudo bash -c 'echo "#{mark}" >> /dev/kmsg'}
  run_command_and_stop(sanitize_text(cmd), fail_on_error: true)
end

Then("the last {int} lines in the kernel ring buffer should contain exactly:") do |int, string|
  lines = int
  expected_output = string
  cmd = %Q{bash -c 'dmesg -t | tail -#{lines}'}
  step %Q{I run `#{cmd}`}
  expect(last_command_stopped).to have_output an_output_string_being_eq(expected_output)
end
