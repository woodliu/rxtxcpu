Feature: direction via invocation

  Direction can also be set via invocation. The `--direction=DIRECTION` option
  takes precedence over the invocation method.

  Scenario: Invocation `rxcpu`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxcpu lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 ../../rxcpu lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: Invocation `txcpu`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../txcpu lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 ../../txcpu lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: Invocation `rxcpu` with `--direction=rxtx`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../rxcpu --direction rxtx lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 ../../rxcpu --direction rxtx lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """

  Scenario: Invocation `txcpu` with `--direction=rxtx`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 5 ../../txcpu --direction rxtx lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the output from "sudo timeout -s INT 5 ../../txcpu --direction rxtx lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
