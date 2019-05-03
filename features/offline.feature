Feature: offline cpu

  cpu0 is not hot pluggable in some of our environments, so certain tests are
  marked with the RequireHotplugCpu0 tag.

  Scenario: Packets sent on cpu0 are counted as such when cpu1 is offline
    Given I wait 2 seconds for a command to start up
    And I disable cpu 1
    When I run `sudo timeout -s INT 5 ../../rxtxcpu lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the stdout from "sudo timeout -s INT 5 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    12 packets captured total.
    """
    And I enable cpu 1

  @RequireHotplugCpu0
  Scenario: Packets sent on cpu1 are counted as such when cpu0 is offline
    Given I wait 2 seconds for a command to start up
    And I disable cpu 0
    When I run `sudo timeout -s INT 5 ../../rxtxcpu lo` in background
    And I run `taskset -c 1 ping -c3 localhost`
    Then the stdout from "sudo timeout -s INT 5 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu1.
    12 packets captured total.
    """
    And I enable cpu 0

  Scenario: Packets sent on cpu0 are counted as such when cpu1 is offline and flipped online
    Given I wait 2 seconds for a command to start up
    And I disable cpu 1
    When I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    And I run `taskset -c 0 ping -c3 localhost` in background
    And I enable cpu 1
    And I run `taskset -c 1 ping -c1 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    12 packets captured total.
    """

  @RequireHotplugCpu0
  Scenario: Packets sent on cpu1 are counted as such when cpu0 is offline and flipped online
    Given I wait 2 seconds for a command to start up
    And I disable cpu 0
    When I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    And I run `taskset -c 1 ping -c3 localhost` in background
    And I enable cpu 0
    And I run `taskset -c 0 ping -c1 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu1.
    12 packets captured total.
    """

  @RequireHotplugCpu0
  Scenario: Packets sent on cpu0 are counted as such when processed before cpu0 is flipped offline
    Given I wait 2 seconds for a command to start up
    When I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    And I run `taskset -c 1 ping -c1 localhost` in background
    And I run `taskset -c 0 ping -c3 localhost`
    And I disable cpu 0
    And I run `taskset -c 1 ping -c1 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    8 packets captured on cpu1.
    20 packets captured total.
    """
    And I enable cpu 0

  Scenario: Packets sent on cpu1 are counted as such when processed before cpu1 is flipped offline
    Given I wait 2 seconds for a command to start up
    When I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    And I run `taskset -c 0 ping -c1 localhost` in background
    And I run `taskset -c 1 ping -c3 localhost`
    And I disable cpu 1
    And I run `taskset -c 0 ping -c1 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    8 packets captured on cpu0.
    12 packets captured on cpu1.
    20 packets captured total.
    """
    And I enable cpu 1
