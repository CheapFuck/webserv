#include <string>
#include <filesystem>
#include <iostream>

static void addTopBoilerplate(std::string& html)
{
	html.append("<!DOCTYPE html>");
	html.append("<html>");
	html.append("<body>");
}

static void addBotBoilerplate(std::string& html)
{
	html.append("</body>");
	html.append("</html>");
}

// static void addRowToIndex(std::string& html,const std::filesystem::directory_entry& e)
// {
// 	html.append("<dt><a href=\"");
// 	html.append(e.path());
// 	html.append("\">");
// 	html.append(e.path().filename());
// 	html.append("<\a></dt>");
// }

static void addRowToIndex(std::string& html,const std::string& fileName, const std::string& publicPath)
{
	html.append("<dt><a href=\"");
	html.append(publicPath);
	html.append(fileName);
	html.append("\">");
	html.append(fileName);
	html.append("</dt></a>");
}

static void addEntriesTable(std::string& html, const std::string& dir_path, const std::string& publicPath)
{
	html.append("<dl>");
	for (const auto & entry : std::filesystem::directory_iterator(dir_path))
	{
		addRowToIndex(html, entry.path().filename(), publicPath);
	}
	html.append("</dl>");
}

std::string redactDirectoryListing(const std::string& dir_path, const std::string& publicPath)
{
	std::string html;

	addTopBoilerplate(html);
	html.append("Index of ");
	html.append(publicPath);	
	html.append("<br><br>");	
	addEntriesTable(html, dir_path, publicPath);
	addBotBoilerplate(html);
	return (html);
}
