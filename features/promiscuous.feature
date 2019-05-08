Feature: `--promiscuous` option

  Use the `--promiscuous` option to ensure the interface is put into
  promiscuous mode before the capture begins.

  Using PACKET_MR_PROMISC is preferable to SIOCGIFFLAGS. However, promiscuity
  requested by PACKET_MR_PROMISC will not show up in SIOCGIFFLAGS.

    See: http://lkml.iu.edu/hypermail/linux/kernel/0101.2/1349.html

  Because of this, we can't use userspace utils to validate promiscuous mode.
  We can, however, validate using the kernel ring buffer.

  Scenario: With `--promiscuous`
    Given I mark the kernel ring buffer with "start of promiscuous test"
    When I run `sudo timeout 2 ../../rxtxcpu --promiscuous lo`
    Then the last 3 lines in the kernel ring buffer should contain exactly:
    """
    start of promiscuous test
    device lo entered promiscuous mode
    device lo left promiscuous mode
    """
    And I mark the kernel ring buffer with "end of promiscuous test"

  Scenario: Without `--promiscuous`
    Given I mark the kernel ring buffer with "start of promiscuous test"
    When I run `sudo timeout 2 ../../rxtxcpu lo`
    Then the last 1 lines in the kernel ring buffer should contain exactly:
    """
    start of promiscuous test
    """
    And I mark the kernel ring buffer with "end of promiscuous test"

  Scenario: With `--promiscuous` with ifindex 0 (any interface)
    Given I wait 0.2 seconds for a command to start up
    And I mark the kernel ring buffer with "start of promiscuous test"
    When I run `sudo timeout -s INT 2 ../../rxtxcpu --promiscuous`
    Then the last 1 lines in the kernel ring buffer should contain exactly:
    """
    start of promiscuous test
    """
    And I mark the kernel ring buffer with "end of promiscuous test"
    And the stdout from "sudo timeout -s INT 2 ../../rxtxcpu --promiscuous" should contain "packets captured total"
