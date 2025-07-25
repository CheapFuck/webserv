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

static void addTimeToRowEntry(std::string& html, const std::filesystem::directory_entry& entry)
{
	std::filesystem::file_time_type ftime;
	try
	{
		ftime = std::filesystem::last_write_time(entry);
	}
	catch(const std::filesystem::filesystem_error& e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << "How we got here is some miracle, not the benevolent kind.";
		html.append("1984 year of the horse... ?");
		return ;
	}
	

	// Convert file_time_type to system_clock::time_point manually
	std::chrono::system_clock::time_point sctp =
		std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			ftime - std::filesystem::file_time_type::clock::now()
			+ std::chrono::system_clock::now()
		);

	// Convert to time_t for printing
	std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
	html.append(std::strftime(std::localtime(&cftime)));
}

// <table>
// <tr>
//   <th>Company</th>
//   <th>Contact</th>
//   <th>Country</th>
// </tr>
// <tr>
//   <td>Alfreds Futterkiste</td>
//   <td>Maria Anders</td>
//   <td>Germany</td>
// </tr>
// <tr>
//   <td>Centro comercial Moctezuma</td>
//   <td>Francisco Chang</td>
//   <td>Mexico</td>
// </tr>
// </table> 
static void addRowToIndex(std::string& html,const std::filesystem::directory_entry& e, const std::string& publicPath)
{
	html.append("<tr><td><a href=\"");
	html.append(publicPath);
	html.append("\\");
	html.append(e.path().filename());
	html.append("\">");
	html.append(e.path().filename());
	html.append("</a></td><td>");
	addTimeToRowEntry(html, e);
	html.append("</td></tr>");
}

static void addEntriesTable(std::string& html, const std::string& dir_path, const std::string& publicPath)
{
	html.append("<table><tr><th>File</th><th>Last Modified</th></tr>");
	try
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir_path))
		{
			addRowToIndex(html, entry, publicPath);
		}
	}
	catch(const std::filesystem::filesystem_error& e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << "Error trying to access path " << dir_path;
	}
	html.append("</table>");
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
