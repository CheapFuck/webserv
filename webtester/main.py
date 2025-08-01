from get_browser import get_browser
from run_tests import run_tests
import time

def main():
	print("\n\n ======= TESTING WEBSERV =======\nIf a test fails, the console will output assertion errors and stop there, and the browser will remain open.\n")
	input("Press enter to start testing.")
	browser = get_browser()
	run_tests(browser)
	browser.quit()

def get_incepted():
	try:
		main()
	except Exception as e:
		print(e)
		input("Something went wrong, press enter when youre done reading the assert or other errors.")

if __name__ == "__main__":
	get_incepted()
