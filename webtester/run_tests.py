from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.common.by import By
from webdriver_manager.firefox import GeckoDriverManager
import pathlib
import time
import os
from selenium.common.exceptions import NoSuchElementException
import inspect

REMOTE_SERVER = "http://localhost:8080"
WEBSERV_ROOT = pathlib.Path(__file__).resolve().parent.parent
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
		browser.find_element(By.NAME, "description").send_keys("Optinally filled out")
		browser.find_element(By.ID, "file").send_keys(f"{WEBTESTER_ROOT}/feline.txt")
		browser.find_element(By.XPATH, '//input[@type="submit" and @value="Upload"]').click()
		assert "File(s) uploaded successfully" in browser.page_source
		assert os.path.exists(f"{WEBTESTER_ROOT}/var/www/uploads/feline.txt")
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



def run_tests(browser):
	test_get(browser)
	test_get2(browser)
	test_post(browser)
	test_delete(browser)
