Feature: `--version` option

  Scenario: With `--version`
    When I run `./rxtxcpu --version`
    Then the exit status should be 0
    And the stderr should not contain anything
    And the stdout should contain "rxtxcpu version"

  Scenario: With `-V`
    When I run `./rxtxcpu -V`
    Then the exit status should be 0
    And the stderr should not contain anything
    And the stdout should contain "rxtxcpu version"
