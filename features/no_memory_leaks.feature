Feature: no memory leaks

  Scenario: savefile /dev/null
    When I run `sudo timeout -s INT 2 valgrind --leak-check=yes ../../rxtxcpu -w /dev/null -U lo`
    Then the stdout should contain "0 packets captured total."
    And the stderr should contain "no leaks are possible"

  Scenario: savefile stdout
    When I run `sudo timeout -s INT 2 valgrind --leak-check=yes ../../rxtxcpu -l1 -w - -U lo`
    Then the stderr should contain "0 packets captured total."
    And the stderr should contain "no leaks are possible"
