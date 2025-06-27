from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from webdriver_manager.firefox import GeckoDriverManager
import pathlib
import os

def get_browser():
    project_root = pathlib.Path(__file__).resolve().parent
    os.environ['TMPDIR'] = f"{project_root}/tmp"

    service = Service(f"{project_root}/geckodriver")
    driver = webdriver.Firefox(service=service)
    driver.set_page_load_timeout(2)
    return (driver)
