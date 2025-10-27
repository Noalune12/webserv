#!/bin/bash


set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RESET='\033[0m' # No Color

# Default values
VERBOSE=0
USE_VALGRIND=0
WEBSERV_BIN="../webserv"
VALID_CONFIGS_DIR="../config-files/valid"
INVALID_CONFIGS_DIR="../config-files/invalid"

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Usage function
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Test the webserv configuration parser with valid and invalid config files.

OPTIONS:
    -v, --verbose       Enable verbose output (show parser error messages)
    --valgrind          Run tests with valgrind memory checking
    -h, --help          Display this help message

DIRECTORIES:
    Valid configs:      $VALID_CONFIGS_DIR
    Invalid configs:    $INVALID_CONFIGS_DIR

EXAMPLES:
    $0                  # Run basic tests
    $0 -v               # Run with verbose output
    $0 --valgrind       # Run with valgrind
    $0 -v --valgrind    # Run with both options
EOF
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        --valgrind)
            USE_VALGRIND=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}Unknown option: $1${RESET}"
            usage
            ;;
    esac
done

# Check if webserv binary exists
if [ ! -f "$WEBSERV_BIN" ]; then
    echo -e "${RED}Error: webserv binary not found at $WEBSERV_BIN${RESET}"
    echo "Please build the project first with 'make'"
    exit 1
fi

# Check if directories exist
if [ ! -d "$VALID_CONFIGS_DIR" ]; then
    echo -e "${YELLOW}Warning: Valid configs directory not found: $VALID_CONFIGS_DIR${RESET}"
fi

if [ ! -d "$INVALID_CONFIGS_DIR" ]; then
    echo -e "${YELLOW}Warning: Invalid configs directory not found: $INVALID_CONFIGS_DIR${RESET}"
fi

VALGRIND_CMD=""
VALGRIND_LOG="/tmp/valgrind_webserv_$$.log"
if [ $USE_VALGRIND -eq 1 ]; then
    if ! command -v valgrind &> /dev/null; then
        echo -e "${RED}Error: valgrind not found. Please install valgrind.${RESET}"
        exit 1
    fi
    VALGRIND_CMD="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes --error-exitcode=42 --log-file=$VALGRIND_LOG -s"
    echo -e "${BLUE}Running with valgrind memory checking${RESET}"
fi

# Function to run a single test
run_test() {
    local config_file="$1"
    local should_succeed="$2"
    local test_name=$(basename "$config_file")

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ $VERBOSE -eq 1 ]; then
        echo -e "${BLUE}Testing: $test_name${RESET}"
    fi

    # Run the command and capture output
    local output
    local exit_code

    if [ $USE_VALGRIND -eq 1 ]; then
        output=$($VALGRIND_CMD timeout 3 $WEBSERV_BIN "$config_file" 2>&1 || true)
        exit_code=$?

        # Check valgrind log for errors
        if [ $exit_code -eq 42 ]; then
            echo -e "${RED}✗ $test_name - VALGRIND ERROR${RESET}"
            if [ $VERBOSE -eq 1 ]; then
                echo -e "${YELLOW}Valgrind output:${RESET}"
                cat "$VALGRIND_LOG"
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    else
        # Normal execution with timeout to prevent hanging
        output=$(timeout 2 $WEBSERV_BIN "$config_file" 2>&1 || true)
        exit_code=$?
    fi

    # Check if the result matches expectations
    if [ $should_succeed -eq 1 ]; then
        # Valid config - should succeed (exit code 0 or timeout since server starts)
        if [ $exit_code -eq 0 ] || [ $exit_code -eq 124 ]; then
            echo -e "${GREEN}✓ $test_name - Valid config parsed successfully${RESET}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            echo -e "${RED}✗ $test_name - Valid config FAILED to parse (exit code: $exit_code)${RESET}"
            if [ $VERBOSE -eq 1 ]; then
                echo -e "${YELLOW}Output:${RESET}"
                echo "$output"
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    else
        # Invalid config - should fail (non-zero exit code, but not timeout)
        if [ $exit_code -ne 0 ] && [ $exit_code -ne 124 ]; then
            echo -e "${GREEN}✓ $test_name - Invalid config correctly rejected${RESET}"
            if [ $VERBOSE -eq 1 ]; then
                echo -e "${YELLOW}Error message:${RESET}"
                echo "$output"
            fi
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            echo -e "${RED}✗ $test_name - Invalid config was NOT rejected (exit code: $exit_code)${RESET}"
            if [ $VERBOSE -eq 1 ]; then
                echo -e "${YELLOW}Output:${RESET}"
                echo "$output"
            fi
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    fi
}

# Main test execution
echo -e "${BLUE}======================================${RESET}"
echo -e "${BLUE}  Configuration Parser Test Suite${RESET}"
echo -e "${BLUE}======================================${RESET}"
echo ""

# Test valid configurations
echo -e "${BLUE}Testing valid configurations...${RESET}"
echo ""

valid_count=$(find "$VALID_CONFIGS_DIR" -name "*.conf" 2>/dev/null | wc -l)
if [ $valid_count -eq 0 ]; then
    echo -e "${YELLOW}No valid config files found in $VALID_CONFIGS_DIR${RESET}"
else
    for config in "$VALID_CONFIGS_DIR"/*.conf; do
        if [ -f "$config" ]; then
            run_test "$config" 1
        fi
    done
fi

echo ""

# Test invalid configurations
echo -e "${BLUE}Testing invalid configurations...${RESET}"
echo ""

invalid_count=$(find "$INVALID_CONFIGS_DIR" -name "*.conf" 2>/dev/null | wc -l)
if [ $invalid_count -eq 0 ]; then
    echo -e "${YELLOW}No invalid config files found in $INVALID_CONFIGS_DIR${RESET}"
else
    for config in "$INVALID_CONFIGS_DIR"/*.conf; do
        if [ -f "$config" ]; then
            run_test "$config" 0
        fi
    done
fi

# Cleanup valgrind log
if [ $USE_VALGRIND -eq 1 ] && [ -f "$VALGRIND_LOG" ]; then
    rm -f "$VALGRIND_LOG"
fi

# Print summary
echo ""
echo -e "${BLUE}======================================${RESET}"
echo -e "${BLUE}  Test Summary${RESET}"
echo -e "${BLUE}======================================${RESET}"
echo -e "Total tests:  $TOTAL_TESTS"
echo -e "${GREEN}Passed:       $PASSED_TESTS${RESET}"
echo -e "${RED}Failed:       $FAILED_TESTS${RESET}"
echo ""

if [ $FAILED_TESTS -eq 0 ] && [ $TOTAL_TESTS -gt 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${RESET}"
    exit 0
elif [ $TOTAL_TESTS -eq 0 ]; then
    echo -e "${YELLOW}No tests were run. Please add config files to test.${RESET}"
    exit 1
else
    echo -e "${RED}Some tests failed! ✗${RESET}"
    exit 1
fi

# if the script doesnt work it might be because of the local keyword, which is not interpreted in a POSIX shell
