Feature: fanout_group

  Scenario: fanout_group_id is properly set
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 3 hold-fanout-group-id-zero` in background
    And I run `sudo timeout -s INT 2 ../../rxtxcpu lo` in background
    And I run `ping -i0.2 -c2 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxtxcpu lo" should contain exactly:
    """
    8 packets captured on cpu0.
    0 packets captured on cpu1.
    8 packets captured total.
    """

  Scenario: Simultaneous invocations
    Given I wait 0.2 seconds for a command to start up
    When I run `sudo timeout -s INT 2 ../../rxcpu lo` in background
    And I run `sudo timeout -s INT 2 ../../txcpu lo` in background
    And I run `ping -i0.2 -c2 localhost` on cpu 0
    Then the output from "sudo timeout -s INT 2 ../../rxcpu lo" should contain exactly:
    """
    4 packets captured on cpu0.
    0 packets captured on cpu1.
    4 packets captured total.
    """
    And the output from "sudo timeout -s INT 2 ../../txcpu lo" should contain exactly:
    """
    4 packets captured on cpu0.
    0 packets captured on cpu1.
    4 packets captured total.
    """
