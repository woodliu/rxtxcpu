Feature: `--direction=DIRECTION`

  Use the `--direction=DIRECTION` option to capture only packets in the given
  direction.

  Scenario: With `--direction=rx`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 ../../rxtxcpu --direction rx lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxtxcpu --direction rx lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: With `-d rx`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 ../../rxtxcpu -d rx lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxtxcpu -d rx lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: With `--direction=tx`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 ../../rxtxcpu --direction tx lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxtxcpu --direction tx lo" should contain exactly:
    """
    6 packets captured on cpu0.
    0 packets captured on cpu1.
    6 packets captured total.
    """

  Scenario: With `--direction=rxtx`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 ../../rxtxcpu --direction rxtx lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxtxcpu --direction rxtx lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """

  Scenario: Without `--direction`
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 ../../rxtxcpu lo` in background
    And I run `ping -i0.2 -c3 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    0 packets captured on cpu1.
    12 packets captured total.
    """
