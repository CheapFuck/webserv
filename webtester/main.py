from get_browser import get_browser
from run_tests import run_tests
import time

def main():
	print("If a test fails, the console will output assertion errors and stop there.")

	try:
		browser = get_browser()
		run_tests(browser)
	finally:
		browser.quit()

if __name__ == "__main__":
	main()
