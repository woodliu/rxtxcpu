Feature: cli with offline cpus

  Background: Enable cpu 1 before each test in this file
    Given I enable cpu 1

  Scenario: cpu list with no online cpus
    Given I disable cpu 1
    When I run `./rxtxcpu -l1`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: No online cpus present in cpu list."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: cpu mask with no online cpus
    Given I disable cpu 1
    When I run `./rxtxcpu -m2`
    Then the exit status should be 2
    And the stdout should not contain anything
    And the stderr should contain "rxtxcpu: No online cpus present in cpu mask."
    And the stderr should contain "Usage: rxtxcpu [--help]"

  Scenario: Enable cpu 1 after all tests in this file
    Given I enable cpu 1
