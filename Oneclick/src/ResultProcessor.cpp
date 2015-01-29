#include "ResultProcessor.h"

ResultProcessor::ResultProcessor(std::string path) {
	fs::directory_iterator end;
	fs::directory_iterator dirIter(path);
	if(dirIter == end) throw std::runtime_error("given argument is not a path to existing directory: " + path);
	oneclickLogger << FileLogger::LOG_INFO << "started processing results\n";

	for(; dirIter != end ; dirIter++) {
		if(dirIter.is_directory() && dirIter.name().compare(".") != 0 && dirIter.name().compare("..")) {

			//Creates directory logger. All directory specific info are logged here also.
			FileLogger * dirLogger = new FileLogger(dirIter.path() + "/000_" + dirIter.name() + ".txt");

			//Directory check
			if(checkConfigs(dirIter.path() , dirLogger)) {
				//Directory check unsuccesfull => no processing
				Score dirScore;
				dirScore.setVal(checkErrorsGetScore(dirIter.path() , dirLogger));
				dirScore.setAlgName(dirIter.name());
				scores.push_back(dirScore);
			}
			delete dirLogger;
		}
	}
	writeScores();
	oneclickLogger << FileLogger::LOG_INFO << "finished processing results\n";
}

bool ResultProcessor::checkConfigs(std::string directory , FileLogger * dirLogger) {
	fs::directory_iterator end;
	fs::directory_iterator dirIter(directory);
	if(dirIter == end) throw std::runtime_error("given argument is not a path to existing directory: " + directory);
	oneclickLogger << FileLogger::LOG_INFO << "checking differences in configs in directory: " << directory << "\n";
	*dirLogger << FileLogger::LOG_INFO << "checking differences in configs in directory: " << directory << "\n";

	bool isSampleSet = false;
	int configCount = 0;
	int logCount = 0;
	int badConfigCount = 0;
	std::string sampleConfig;
	std::string currentConfig;
	std::string filePath;

	for(; dirIter != end; dirIter++) {
		if(dirIter.is_file() && dirIter.name().find(IDENTIFIER_CONFIG) != -1) {
			configCount++;
			filePath = dirIter.path();
			if(!isSampleSet) {
				sampleConfig = Utils::readFileToString(filePath);
				isSampleSet = true;
			}

			currentConfig = Utils::readFileToString(filePath);
			if(sampleConfig.compare(currentConfig) != 0) {
				oneclickLogger << FileLogger::LOG_WARNING << "config " << filePath << " differs from the first config in directory\n";
				*dirLogger << FileLogger::LOG_WARNING << "config " << filePath << " differs from the first config in directory\n";
				badConfigCount++;
			}

			filePath.erase();
			currentConfig.erase();
		}

		if(dirIter.is_file() && dirIter.name().find(IDENTIFIER_LOG) != -1) logCount++;
	}

	oneclickLogger << FileLogger::LOG_INFO << Utils::itostr(configCount) << " configs and " << Utils::itostr(logCount) << " logs in directory\n";
	*dirLogger << FileLogger::LOG_INFO << Utils::itostr(configCount) << " configs and " << Utils::itostr(logCount) << " logs in directory\n";

	if(badConfigCount == 0) {
		oneclickLogger << FileLogger::LOG_INFO << "no different configs in directory\n\n";
		*dirLogger << FileLogger::LOG_INFO << "no different configs in directory\n\n";
		return true;
	} else {
		oneclickLogger << FileLogger::LOG_WARNING << Utils::itostr(badConfigCount) << " different configs in directory\n";
		oneclickLogger << FileLogger::LOG_WARNING << "directory " << directory << " won't be processed. Remove invalid runs before processing!\n\n";
		*dirLogger << FileLogger::LOG_WARNING << Utils::itostr(badConfigCount) << " different configs in directory\n";
		*dirLogger << FileLogger::LOG_WARNING << "directory " << directory << " won't be processed. Remove invalid runs before processing!\n\n";
		return false;
	}
	return false;
}

double ResultProcessor::checkErrorsGetScore(std::string directory , FileLogger * dirLogger) {
	fs::directory_iterator end;
	fs::directory_iterator dirIter(directory);
	if(dirIter == end) throw std::runtime_error("given argument is not a path to existing directory: " + directory);

	oneclickLogger << FileLogger::LOG_INFO << "getting score, checking errors/warnings in logs: " << directory << "\n\n";
	*dirLogger << FileLogger::LOG_INFO << "getting score, checking errors/warnings in logs: " << directory << "\n\n";

	int errorCount = 0;
	int wrnCount = 0;
	int validFileCount = 0;
	int uniformFileCount = 0;
	bool uniformity;
	bool hasResult;
	bool validity;
	std::string filePath;

	std::regex err   ("\\[\\d\\d:\\d\\d:\\d\\d\\] error: .*");
	std::regex wrn   ("\\[\\d\\d:\\d\\d:\\d\\d\\] warning: .*");
	std::regex uni   ("\\[\\d\\d:\\d\\d:\\d\\d\\] info:    KS is not in 5% interval -> is uniform\\.");
	std::regex nonUni("\\[\\d\\d:\\d\\d:\\d\\d\\] info:    KS is in 5% interval -> uniformity hypothesis rejected\\.");

	std::sregex_token_iterator endExpr;
	std::string logFile;

	for(; dirIter != end ; dirIter++) {
		if(dirIter.is_file() && dirIter.name().find(IDENTIFIER_LOG) != -1) {
			filePath = dirIter.path();
			errorCount = 0;
			wrnCount = 0;
			uniformity = false;
			hasResult = false;
			validity = true;
			logFile = Utils::readFileToString(filePath);

			std::sregex_token_iterator errors(logFile.begin() , logFile.end() , err , 0);
			std::sregex_token_iterator warnings(logFile.begin() , logFile.end() , wrn , 0);
			std::sregex_token_iterator uniform(logFile.begin() , logFile.end() , uni , 0);
			std::sregex_token_iterator nonUniform(logFile.begin() , logFile.end() , nonUni , 0);

			for(; errors != endExpr ; errors++) {
				errorCount++;
				*dirLogger << FileLogger::LOG_WARNING << "error in log file: " << filePath << " == " << *errors << "\n";
				oneclickLogger << FileLogger::LOG_WARNING << "error in log file: " << filePath << " == " << *errors << "\n";
			}

			for(; warnings != endExpr ; warnings++) {
				wrnCount++;
				*dirLogger << FileLogger::LOG_WARNING << "warning in log file: " << filePath << " == " << *warnings << "\n";
				oneclickLogger << FileLogger::LOG_WARNING << "warning in log file: " << filePath << " == " << *warnings << "\n";
			}

			if(uniform != endExpr && nonUniform == endExpr) {
				uniformity = true;
				hasResult = true;
		    }

			if(uniform == endExpr && nonUniform != endExpr) {
				uniformity = false;
				hasResult = true;
			}

			if(uniform != endExpr && nonUniform != endExpr) {
				validity = false;
			}

			if(validity) {
				if(errorCount == 0) {
					if(hasResult) {
						validFileCount++;
						if(uniformity) {
							uniformFileCount++;
						}
					} else {
						*dirLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n";
						oneclickLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n";
						*dirLogger << FileLogger::LOG_WARNING << filePath << " contains no result. Ignoring file.\n\n";
						oneclickLogger << FileLogger::LOG_WARNING << filePath << " contains no result. Ignoring file.\n\n";
					}
				} else {
					*dirLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n";
					oneclickLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n";
					*dirLogger << FileLogger::LOG_WARNING << filePath << " contains errors. Ignoring file.\n\n";
					oneclickLogger << FileLogger::LOG_WARNING << filePath << " contains errors. Ignoring file.\n\n";
				}

				if(errorCount == 0 && wrnCount != 0 && hasResult) {
					*dirLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n\n";
					oneclickLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n\n";
				}
			} else {
				*dirLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n\n";
				oneclickLogger << FileLogger::LOG_INFO << filePath << " has " << Utils::itostr(wrnCount) << " warnings and " << Utils::itostr(errorCount) << " errors.\n\n";
				*dirLogger << FileLogger::LOG_WARNING << filePath << " contains two or more inconsistent results. Ignoring file.\n\n";
				oneclickLogger << FileLogger::LOG_WARNING << filePath << "contains two or more inconsistent results. Ignoring file.\n\n";
			}
			logFile.erase();
		}
	}
	if(validFileCount != 0) {
		return (float)uniformFileCount / (float)validFileCount;
	} else {
		return ERROR_NO_VALID_FILES;
	}
	return 1.0;
}

void ResultProcessor::writeScores() {
	std::ofstream resultFile(FILE_PROCESSED_RESULTS , std::ios::out);
	if(!resultFile.is_open()) throw std::runtime_error("can't open file: " + (std::string)FILE_PROCESSED_RESULTS);

	for(int i = 0 ; i < scores.size() ; i++) {
		resultFile << scores[i].toString() << "\n";
	}

	resultFile.close();
}