#include "DukeApplication.hpp"

#include <duke/engine/streams/DiskMediaStream.hpp>
#include <duke/engine/overlay/DukeSplashStream.hpp>
#include <duke/cmdline/CmdLineParameters.hpp>
#include <duke/imageio/DukeIO.hpp>
#include <duke/filesystem/FsUtils.hpp>
#include <duke/gl/GL.hpp>

#include <sequence/Parser.hpp>

#include <glm/glm.hpp>

#include <memory>

namespace duke {

namespace {

sequence::Configuration getParserConf() {
	using namespace sequence;
	Configuration conf;
	conf.sort = true;
	conf.bakeSingleton = true;
	conf.mergePadding = true;
	conf.pack = true;
	return conf;
}

bool isValid(const std::string& filename) {
	if (!filename.empty() && filename[0] == '.')
		return false;
	const char* pExtension = fileExtension(filename.c_str());
	if (!pExtension)
		return false;
	if (!IODescriptors::instance().isSupported(pExtension))
		return false;
	return true;
}

}  // namespace

Timeline buildTimeline(const std::vector<std::string> &paths) {
	using sequence::Item;
	Track track;
	size_t offset = 0;
	for (const std::string &path : paths) {
		const std::string absolutePath = getAbsoluteFilename(path.c_str());
		switch (getFileStatus(absolutePath.c_str())) {
		case FileStatus::NOT_A_FILE:
			throw commandline_error("'" + absolutePath + "' is not a file nor a directory");
		case FileStatus::FILE:
			track.add(offset, Clip { 1, std::make_shared<DiskMediaStream>(Item(absolutePath)), nullptr });
			++offset;
			break;
		case FileStatus::DIRECTORY:
			for (Item item : sequence::parseDir(getParserConf(), absolutePath.c_str()).files) {
				const auto type = item.getType();
				if (type == Item::INVALID)
					throw commandline_error("invalid item while parsing directory");
				if (!isValid(item.filename))
					continue; // escaping hidden file
				item.filename = absolutePath + '/' + item.filename;
				switch (type) {
				case Item::SINGLE:
					track.add(offset, Clip { 1, std::make_shared<DiskMediaStream>(item), nullptr });
					++offset;
					break;
				case Item::PACKED: {
					const auto count = item.end - item.start + 1;
					track.add(offset, Clip { count, std::make_shared<DiskMediaStream>(item), nullptr });
					offset += count;
				}
				case Item::INDICED:
				default:
					break;
				}
			}
			break;
		}
	}
	return {track};
}

Timeline buildDemoTimeline() {
	Timeline timeline;
	const std::string exePath = getDirname(getExePath());
	const std::string splashScreenPath = exePath + "/splashscreen";
	if (getFileStatus(splashScreenPath.c_str()) == FileStatus::DIRECTORY) {
		timeline = buildTimeline( { splashScreenPath });
	}
	const Range range = timeline.empty() ? Range(0, 0) : timeline.getRange();
	Track overlay;
	overlay.add(range.first, Clip { range.count(), nullptr, std::make_shared<DukeSplashStream>() });
	timeline.push_back(overlay);
	return timeline;
}

namespace {

GLFWwindow * initializeMainWindow(DukeGLFWApplication *pApplication, const CmdLineParameters &parameters) {
	const bool fullscreen = parameters.fullscreen;
	GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
	if (pPrimaryMonitor == nullptr) {
		int monitors = 0;
		GLFWmonitor **pMonitors = nullptr;
		pMonitors = glfwGetMonitors(&monitors);
		if (monitors == 0 || pMonitors == nullptr)
			throw std::runtime_error("No monitor detected");
		pPrimaryMonitor = pMonitors[0];
	}
	const GLFWvidmode* desktopDefinition = glfwGetVideoMode(pPrimaryMonitor);
	auto windowDefinition = glm::ivec2(desktopDefinition->width, desktopDefinition->height);
	if (!fullscreen)
		windowDefinition /= 2;
	return pApplication->createRawWindow(windowDefinition.x, windowDefinition.y, "", fullscreen ? pPrimaryMonitor : nullptr, nullptr);
}

}  // namespace

DukeApplication::DukeApplication(const CmdLineParameters &parameters) :
		m_MainWindow(initializeMainWindow(this, parameters), parameters) {

	auto timeline = buildTimeline(parameters.additionnalOptions);
	auto frameDuration = parameters.defaultFrameRate;
	auto fitMode = FitMode::INNER;
	auto speed = 0;
	const bool demoMode = timeline.empty();
	if (demoMode) {
		timeline = buildDemoTimeline();
		frameDuration = FrameDuration(1, 15);
		fitMode = FitMode::OUTER;
		speed = 1;
	}

	m_MainWindow.load(timeline, frameDuration, fitMode, speed);
}

void DukeApplication::run() {
	m_MainWindow.run();
}

} /* namespace duke */
