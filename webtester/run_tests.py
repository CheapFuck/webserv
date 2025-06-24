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

def test_get(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/get")
	assert "get" in browser.title
	assert "OH THE BODY" in browser.page_source

def test_get2(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/get/faaa/wwWWWw/garbagEEE")
	assert "404" in browser.title
	assert "NotFound" in browser.title
	assert "Error 404 - NotFound" in browser.page_source

def test_post(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/post/new")
	try:
		browser.find_element(By.NAME, "name").send_keys("Some name")
		browser.find_element(By.ID, "submit").click()
		assert "created" in browser.page_source.lower()
	except NoSuchElementException as e:
		print(f"Element not found in test_post")

def test_put(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/put/1/edit")
	try:
		field = browser.find_element(By.NAME, "name")
		field.clear()
		field.send_keys("Updated")
		browser.find_element(By.ID, "submit").click()
		assert "updated" in browser.page_source.lower()
	except NoSuchElementException as e:
		print(f"Element not found in test_put")

def test_delete(browser):
	print(f"Running function {inspect.stack()[0][3]}")
	browser.get(f"{REMOTE_SERVER}/delete/1")
	try:
		browser.find_element(By.ID, "delete").click()
		assert "deleted" in browser.page_source.lower()
	except NoSuchElementException as e:
		print(f"Element not found in test_delete")



def run_tests(browser):
	test_get(browser)
	test_get2(browser)
	test_post(browser)
	test_put(browser)
	test_delete(browser)
