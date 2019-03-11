Feature: offline cpu

  `chcpu` is not available in trusty. `/sys/devices/system/cpu/cpu*/online` is
  not available on ubuntu. `/sys/devices/system/node/node*/cpu*/online` is
  available in all our current test environments. We'll use that.

  cpu0 is not hot pluggable in some of our environments, so certain tests are
  marked with the RequireHotplugCpu0 tag.

  Scenario: Packets sent on cpu0 are counted as such when cpu1 is offline
    Given I wait 2 seconds for a command to start up
    # The following is equivalent to `sudo chcpu -d 1`
    When I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu1/online'`
    And I run `sudo timeout -s INT 5 ../../rxtxcpu lo` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the stdout from "sudo timeout -s INT 5 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    12 packets captured total.
    """
    # The following is equivalent to `sudo chcpu -e 1`
    And I run `bash -c 'echo 1 | sudo tee /sys/devices/system/node/node0/cpu1/online'`

  @RequireHotplugCpu0
  Scenario: Packets sent on cpu1 are counted as such when cpu0 is offline
    Given I wait 2 seconds for a command to start up
    # The following is equivalent to `sudo chcpu -d 0`
    When I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu0/online'`
    And I run `sudo timeout -s INT 5 ../../rxtxcpu lo` in background
    And I run `taskset -c 1 ping -c3 localhost`
    Then the stdout from "sudo timeout -s INT 5 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu1.
    12 packets captured total.
    """
    # The following is equivalent to `sudo chcpu -e 0`
    And I run `bash -c 'echo 1 | sudo tee /sys/devices/system/node/node0/cpu0/online'`

  Scenario: Packets sent on cpu0 are counted as such when cpu1 is offline and flipped online
    Given I wait 2 seconds for a command to start up
    # The following is equivalent to `sudo chcpu -d 1`
    When I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu1/online'`
    And I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    # The following is equivalent to `bash -c 'sleep 1; sudo chcpu -e 1` in background
    And I run `bash -c 'sleep 1; echo 1 | sudo tee /sys/devices/system/node/node0/cpu1/online'` in background
    And I run `bash -c 'sleep 2; taskset -c 1 ping -c1 localhost'` in background
    And I run `taskset -c 0 ping -c3 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    12 packets captured total.
    """

  @RequireHotplugCpu0
  Scenario: Packets sent on cpu1 are counted as such when cpu0 is offline and flipped online
    Given I wait 2 seconds for a command to start up
    # The following is equivalent to `sudo chcpu -d 0`
    When I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu0/online'`
    And I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    # The following is equivalent to `bash -c 'sleep 1; sudo chcpu -e 0` in background
    And I run `bash -c 'sleep 1; echo 1 | sudo tee /sys/devices/system/node/node0/cpu0/online'` in background
    And I run `bash -c 'sleep 2; taskset -c 0 ping -c1 localhost'` in background
    And I run `taskset -c 1 ping -c3 localhost`
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
    # The following is equivalent to `sudo chcpu -d 0`
    And I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu0/online'`
    And I run `taskset -c 1 ping -c1 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    12 packets captured on cpu0.
    8 packets captured on cpu1.
    20 packets captured total.
    """
    # The following is equivalent to `sudo chcpu -e 0`
    And I run `bash -c 'echo 1 | sudo tee /sys/devices/system/node/node0/cpu0/online'`

  Scenario: Packets sent on cpu1 are counted as such when processed before cpu1 is flipped offline
    Given I wait 2 seconds for a command to start up
    When I run `sudo timeout -s INT 10 ../../rxtxcpu lo` in background
    And I run `taskset -c 0 ping -c1 localhost` in background
    And I run `taskset -c 1 ping -c3 localhost`
    # The following is equivalent to `sudo chcpu -d 1`
    And I run `bash -c 'echo 0 | sudo tee /sys/devices/system/node/node0/cpu1/online'`
    And I run `taskset -c 0 ping -c1 localhost`
    Then the stdout from "sudo timeout -s INT 10 ../../rxtxcpu lo" should contain exactly:
    """
    8 packets captured on cpu0.
    12 packets captured on cpu1.
    20 packets captured total.
    """
    # The following is equivalent to `sudo chcpu -e 1`
    And I run `bash -c 'echo 1 | sudo tee /sys/devices/system/node/node0/cpu1/online'`
