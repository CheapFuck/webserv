from get_browser import get_browser
from run_tests import run_tests
import time

def main():
	browser = get_browser()

	try:
		run_tests(browser)
		try:
			while True:
				time.sleep(1)
		except KeyboardInterrupt:
			pass
	finally:
		browser.quit()
if __name__ == "__main__":
	main()
