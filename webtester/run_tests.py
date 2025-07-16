from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from get_browser import get_browser
from selenium.webdriver.common.by import By
from webdriver_manager.firefox import GeckoDriverManager
import pathlib
import time
import os
import tempfile
from selenium.common.exceptions import NoSuchElementException
import inspect
from concurrent.futures import ThreadPoolExecutor, as_completed
import json

REMOTE_SERVER = "http://localhost:8080"
WEBTESTER_ROOT = pathlib.Path(__file__).resolve().parent

def test_get(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/home")
	assert "Our Showcase" in browser.title
	assert "your own HTTP server." in browser.page_source

def test_get2(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/")
	assert "404" in browser.title
	assert "NotFound" in browser.title
	assert "Error 404 - NotFound" in browser.page_source

def test_post(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/home/upload_form.html")
	try:
		os.remove(f"{WEBTESTER_ROOT}/var/www/uploads/feline.txt")
	except:
		pass
	try:
		browser.find_element(By.NAME, "description").send_keys("Optionally filled out")
		browser.find_element(By.ID, "file").send_keys(f"{WEBTESTER_ROOT}/feline.txt")
		browser.find_element(By.XPATH, '//input[@type="submit" and @value="Upload"]').click()
		assert "File(s) uploaded successfully" in browser.page_source
		assert os.path.exists(f"{WEBTESTER_ROOT}/var/www/uploads/feline.txt")
		browser.get(f"{REMOTE_SERVER}/view_uploaded/feline.txt")
		time.sleep(1)
	except NoSuchElementException as e:
		print(f"Element not found in test_post")

def test_post_upload_not_existing(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/home/upload_form.html")
	try:
		browser.find_element(By.NAME, "description").send_keys("Optinally filled out")
		with tempfile.NamedTemporaryFile(delete=True) as temp_file:
			browser.find_element(By.ID, "file").send_keys(temp_file.name)
			temp_file.flush()
		browser.find_element(By.XPATH, '//input[@type="submit" and @value="Upload"]').click()
		assert "No files uploaded" in browser.page_source
	except NoSuchElementException as e:
		print(f"Element not found in test_post")

def test_post_large_file(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/home/upload_form.html")
	try:
		browser.find_element(By.NAME, "description").send_keys("Optinally filled out")
		with tempfile.NamedTemporaryFile(delete=True) as temp_file:
			temp_file.write(b'A' * 1024 * 1024 * 3)
			temp_file.flush()
			browser.find_element(By.ID, "file").send_keys(temp_file.name)
			browser.find_element(By.XPATH, '//input[@type="submit" and @value="Upload"]').click()
		assert "413 - PayloadTooLarge" in browser.title
		assert "Error 413 - PayloadTooLarge" in browser.page_source
	except NoSuchElementException as e:
		print(f"Element not found in test_post")

def test_delete(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/home/delete_form.html")
	try:
		browser.find_element(By.ID, "filePath").send_keys(f"feline.txt")
		assert os.path.exists(f"{WEBTESTER_ROOT}/var/www/uploads/feline.txt")
		browser.find_element(By.XPATH, '//button[@type="submit"]').click()
		time.sleep(1)
		assert (browser.switch_to.alert.text == "File deleted successfully")
		assert not os.path.exists(f"{WEBTESTER_ROOT}/var/www/uploads/feline.txt")
		browser.switch_to.alert.accept()
	except NoSuchElementException as e:
		print(f"Element not found in test_delete")

def test_delete_doesntexist(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/home/delete_form.html")
	try:
		browser.find_element(By.ID, "filePath").send_keys(f"feline.txt")
		assert not os.path.exists(f"{WEBTESTER_ROOT}/var/www/uploads/feline.txt")
		browser.find_element(By.XPATH, '//button[@type="submit"]').click()
		time.sleep(1)
		assert (browser.switch_to.alert.text == "File not found")
		browser.switch_to.alert.accept()
	except NoSuchElementException as e:
		print(f"Element not found in test_delete")

def spamalittle(browser):
	spamount = 200
	print(f"Spamming with {spamount} + 1 get requests to the debug cgi endpoint")
	results = list()
	with ThreadPoolExecutor(max_workers=spamount) as executor:
		futures = [executor.submit(test_get_cgi, browser) for i in range(spamount)]

		for future in as_completed(futures):
			try:
				result = future.result()  # This gets the return value or raises any exception
				results.append(result)
			except Exception as e:
				print(f"Unexpected error in thread: {e}")
				results.append(False)
	test_get_cgi(browser)
	assert "{\"debugVisitCounter\": 201}," in browser.page_source


def test_get_cgi(browser):
	browser.get(f"{REMOTE_SERVER}/cgi/debug")

def spamalittle_twosessions(browser):
	spamount = 50
	print(f"Spamming with {spamount * 2} get requests to the debug cgi endpoint")
	browser_two = get_browser()
	for i in range(spamount):
		test_get_cgi(browser)
		test_get_cgi(browser_two)
		
	time.sleep(2)
	print(repr(browser.page_source))
	data = json.loads(repr(browser.page_source))
	data1 = json.loads(repr(browser_two.page_source))
	assert (data['sessionData']['debugVisitCount'] == 50)
	assert (data1['sessionData']['debugVisitCount'] == 50)
	assert (data1['envVars']['SERVER_PORT'] == "8080" == data['envVars']['SERVER_PORT'])
	printf(f"Both browsers/sessions hit {spamount} counts of visits to the debug page.")
	# check sessoin files stored ../ are matching
	browser_two.quit()

def run_tests(browser):
	test_get(browser)
	test_get2(browser)
	test_post(browser)
	test_post_upload_not_existing(browser)
	test_post_large_file(browser)
	test_delete(browser)
	test_delete_doesntexist(browser)
	spamalittle_twosessions(browser)
	spamalittle(browser)

