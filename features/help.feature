Feature: `--help` option

  Scenario: With `--help`
    When I run `./rxtxcpu --help`
    Then the exit status should be 0
    And the stderr should not contain anything
    And the stdout should contain "Usage:"
    And the stdout should contain "rxtxcpu"
    And the stdout should contain "Options:"
    And the stdout should contain "[--help]"

  Scenario: With `-h`
    When I run `./rxtxcpu -h`
    Then the exit status should be 0
    And the stderr should not contain anything
    And the stdout should contain "Usage:"
    And the stdout should contain "rxtxcpu"
    And the stdout should contain "Options:"
    And the stdout should contain "[--help]"
