#include "EstreamProject.h"
#include "EstreamConstants.h"
#include "AvalancheEvaluator.h"

ESTREAM_SETTINGS* pEstreamSettings = NULL;

// TODO clean-up initialization section
EstreamProject::EstreamProject()
    : IProject(PROJECT_ESTREAM), m_tvOutputs(NULL), m_tvInputs(NULL), m_plaintextIn(NULL), m_plaintextOut(NULL),
      m_numVectors(NULL), m_encryptorDecryptor(NULL) {
    m_tvOutputs = new unsigned char[pGlobals->settings->testVectors.outputLength];
    memset(m_tvOutputs,0,pGlobals->settings->testVectors.outputLength);
    m_tvInputs = new unsigned char[pGlobals->settings->testVectors.inputLength];
    memset(m_tvInputs,0,pGlobals->settings->testVectors.inputLength);
    m_plaintextIn = new unsigned char[pGlobals->settings->testVectors.inputLength];
    m_plaintextOut = new unsigned char[pGlobals->settings->testVectors.inputLength];
    m_numVectors = new int[2];
}

EstreamProject::~EstreamProject() {
    if (m_tvOutputs) delete[] m_tvOutputs;
    m_tvOutputs = NULL;
    if (m_tvInputs) delete[] m_tvInputs;
    m_tvInputs = NULL;
    if (m_plaintextIn) delete[] m_plaintextIn;
    m_plaintextIn = NULL;
    if (m_plaintextOut) delete[] m_plaintextOut;
    m_plaintextOut = NULL;
    if (m_numVectors) delete[] m_numVectors;
    m_numVectors = NULL;
    if (m_encryptorDecryptor) delete m_encryptorDecryptor;
    m_encryptorDecryptor = NULL;
}

string EstreamProject::shortDescription() const {
    return "eStream candidate ciphers";
}

int EstreamProject::loadProjectConfiguration(TiXmlNode* pRoot) {
    m_estreamSettings.usageType = atoi(getXMLElementValue(pRoot,"ESTREAM/USAGE_TYPE").c_str());
    m_estreamSettings.algorithm1 = atoi(getXMLElementValue(pRoot,"ESTREAM/ALGORITHM_1").c_str());
    m_estreamSettings.algorithm2 = atoi(getXMLElementValue(pRoot,"ESTREAM/ALGORITHM_2").c_str());
    m_estreamSettings.ballancedTestVectors = (atoi(getXMLElementValue(pRoot,"ESTREAM/BALLANCED_TEST_VECTORS").c_str())) ? true : false;
    m_estreamSettings.limitAlgRounds = (atoi(getXMLElementValue(pRoot,"ESTREAM/LIMIT_NUM_OF_ROUNDS").c_str())) ? true : false;
    m_estreamSettings.alg1RoundsCount = atoi(getXMLElementValue(pRoot,"ESTREAM/ROUNDS_ALG_1").c_str());
    m_estreamSettings.alg2RoundsCount = atoi(getXMLElementValue(pRoot,"ESTREAM/ROUNDS_ALG_2").c_str());
    m_estreamSettings.plaintextType= atoi(getXMLElementValue(pRoot,"ESTREAM/PLAINTEXT_TYPE").c_str());
    m_estreamSettings.keyType = atoi(getXMLElementValue(pRoot,"ESTREAM/KEY_TYPE").c_str());
    m_estreamSettings.ivType = atoi(getXMLElementValue(pRoot,"ESTREAM/IV_TYPE").c_str());
    m_estreamSettings.cipherInitializationFrequency = atoi(getXMLElementValue(pRoot,"ESTREAM/CIPHER_INIT_FREQ").c_str());
    m_estreamSettings.generateStream = (atoi(getXMLElementValue(pRoot,"ESTREAM/GENERATE_STREAM").c_str())) ? true : false;
    istringstream ss(getXMLElementValue(pRoot,"ESTREAM/STREAM_SIZE"));
    ss >> m_estreamSettings.streamSize;
    pEstreamSettings = &m_estreamSettings;

    // bind project settings into global settings
    pGlobals->settings->project = (void*) pEstreamSettings;

    return STAT_OK;
}

int EstreamProject::initializeProject() {
    // allocate encryptorDecryptor
    m_encryptorDecryptor = new EncryptorDecryptor;
    // create test vector headers, if saving test vectors
    if (pGlobals->settings->testVectors.saveTestVectors) {
        // generate header (project config) to test vector file
        ofstream tvFile;
        tvFile.open(FILE_TEST_VECTORS, ios_base::app | ios_base::binary);
        if (!tvFile.is_open()) {
            mainLogger.out(LOGGER_ERROR) << "Cannot write file for test vectors (" << FILE_TEST_VECTORS << ")." << endl;
            return STAT_FILE_WRITE_FAIL;
        }
        tvFile << pGlobals->settings->main.projectType << " \t\t(project: " << shortDescription() << ")" << endl;
        tvFile << pEstreamSettings->usageType << " \t\t(usage type)" << endl;
        tvFile << pEstreamSettings->cipherInitializationFrequency << " \t\t(cipher initialization frequency)" << endl;
        tvFile << pEstreamSettings->algorithm1 << " \t\t(algorithm1: " << estreamToString(pEstreamSettings->algorithm1) << ")" << endl;
        tvFile << pEstreamSettings->algorithm2 << " \t\t(algorithm2: " << estreamToString(pEstreamSettings->algorithm2) << ")" << endl;
        tvFile << pEstreamSettings->ballancedTestVectors << " \t\t(ballanced test vectors?)" << endl;
        tvFile << pEstreamSettings->limitAlgRounds << " \t\t(limit algorithm rounds?)" << endl;
        if (pEstreamSettings->limitAlgRounds) {
            tvFile << pEstreamSettings->alg1RoundsCount << " \t\t(algorithm1: " << pEstreamSettings->alg1RoundsCount << " rounds)" << endl;
            tvFile << pEstreamSettings->alg2RoundsCount << " \t\t(algorithm2: " << pEstreamSettings->alg2RoundsCount << " rounds)" << endl;
        }
        tvFile << pEstreamSettings->plaintextType << " \t\t(plaintext type)" << endl;
        tvFile << pEstreamSettings->keyType << " \t\t(key type)" << endl;
        tvFile << pEstreamSettings->ivType << " \t\t(IV type)" << endl;
        tvFile.close();

        // generate header to human-readable test-vector file
        tvFile.open(FILE_TEST_VECTORS_HR, ios::app | ios_base::binary);
        tvFile << "Using eStream ciphers and random generator to generate test vectors." << endl;
        tvFile << "  stream1: using " << estreamToString(pEstreamSettings->algorithm1);
        if (pEstreamSettings->limitAlgRounds) {
            tvFile << " (" << pEstreamSettings->alg1RoundsCount << " rounds)" << endl;
        } else {
            tvFile << " (unlimited version)" << endl;
        }
        tvFile << "  stream2: using " << estreamToString(pEstreamSettings->algorithm2);
        if (pEstreamSettings->limitAlgRounds) {
            tvFile << " (" << pEstreamSettings->alg2RoundsCount << " rounds)" << endl;
        } else {
            tvFile << " (unlimited version)" << endl;
        }
        tvFile << "Test vectors formatted as PLAINTEXT::CIPHERTEXT::DECRYPTED" << endl;
        tvFile.close();
    }
    // allocate project-specific evaluator, if needed
    if (pGlobals->settings->main.evaluatorType == ESTREAM_EVALUATOR_AVALANCHE) {
        m_projectEvaluator = new AvalancheEvaluator(m_encryptorDecryptor);
    }
    return STAT_OK;
}

int EstreamProject::initializeProjectState() {
    int status = STAT_OK;
    if (pEstreamSettings->cipherInitializationFrequency == ESTREAM_INIT_CIPHERS_ONCE) {
        status = m_encryptorDecryptor->setupKey();
        if (status != STAT_OK) return status;
        status = m_encryptorDecryptor->setupIV();
    }
    return status;
}

int EstreamProject::setupPlaintext() {
    switch (pEstreamSettings->plaintextType) {
    case ESTREAM_GENTYPE_ZEROS:
        for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) m_plaintextIn[input] = 0x00;
        break;
    case ESTREAM_GENTYPE_ONES:
        for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) m_plaintextIn[input] = 0x01;
        break;
    case ESTREAM_GENTYPE_RANDOM:
        for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) rndGen->getRandomFromInterval(255, &(m_plaintextIn[input]));
        break;
    case ESTREAM_GENTYPE_BIASRANDOM:
        for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) biasRndGen->getRandomFromInterval(255, &(m_plaintextIn[input]));
        break;
    default:
        mainLogger.out(LOGGER_ERROR) << "Unknown plaintext type for " << shortDescription() << endl;
        return STAT_INVALID_ARGUMETS;
    }

    return STAT_OK;
}

int EstreamProject::saveProjectState(TiXmlNode* pRoot) const {
    TiXmlElement* pRoot2 = pRoot->ToElement();
    TiXmlElement* pNode;
    if (m_estreamSettings.cipherInitializationFrequency != ESTREAM_INIT_CIPHERS_ONCE) {
        pRoot2->SetAttribute("loadable",1);
    } else {
        ostringstream ssKeyAndIV;

        pNode = new TiXmlElement("key");
        for (int input = 0; input < STREAM_BLOCK_SIZE; input++)
        ssKeyAndIV << setw(2) << hex << (int)(m_encryptorDecryptor->m_key[input]);
        pNode->LinkEndChild(new TiXmlText(ssKeyAndIV.str().c_str()));
        pRoot2->LinkEndChild(pNode);

        ssKeyAndIV.str("");
        pNode = new TiXmlElement("iv");
        for (int input = 0; input < STREAM_BLOCK_SIZE; input++)
        ssKeyAndIV << setw(2) << hex << (int)(m_encryptorDecryptor->m_iv[input]);
        pNode->LinkEndChild(new TiXmlText(ssKeyAndIV.str().c_str()));
        pRoot2->LinkEndChild(pNode);
    }
    return STAT_OK;
}

int EstreamProject::loadProjectState(TiXmlNode* pRoot) {

    return STAT_OK;
}

int EstreamProject::generateTestVectors() {
    int status = STAT_OK;

    // if set so, do not generate test vectors but generate data stream to cout
    if (pEstreamSettings->generateStream) {
        status = generateCipherDataStream();
        if (status != STAT_OK) {
            return status;
        } else {
            return STAT_INTENTIONAL_EXIT;
        }
    }

    if (pEstreamSettings->cipherInitializationFrequency == ESTREAM_INIT_CIPHERS_FOR_SET) {
        m_encryptorDecryptor->setupKey();
        m_encryptorDecryptor->setupIV();
    }

    // USED FOR BALANCING TEST VECTORS
    this->m_numVectors[0] = 0;
    this->m_numVectors[1] = 0;

    for (int testVectorNumber = 0; testVectorNumber < pGlobals->settings->testVectors.setSize; testVectorNumber++) {
        if (pGlobals->settings->testVectors.saveTestVectors == 1) {
            ofstream tvfile(FILE_TEST_VECTORS_HR, ios::app);
            tvfile << "Test vector n." << dec << testVectorNumber << endl;
            tvfile.close();
        }

        if (pEstreamSettings->cipherInitializationFrequency == ESTREAM_INIT_CIPHERS_FOR_VECTOR) {
            m_encryptorDecryptor->setupKey();
            m_encryptorDecryptor->setupIV();
        }
        status = getTestVector();
        if (status != STAT_OK) {
            return status;
        }

        for (int inputByte = 0; inputByte < pGlobals->settings->testVectors.inputLength; inputByte++) {
            pGlobals->testVectors.inputs[testVectorNumber][inputByte] = m_tvInputs[inputByte];
        }
        for (int outputByte = 0; outputByte < pGlobals->settings->testVectors.outputLength; outputByte++)
            pGlobals->testVectors.outputs[testVectorNumber][outputByte] = m_tvOutputs[outputByte];
    }
    return status;
}

int EstreamProject::getTestVector(){
    int status = STAT_OK;
    ofstream tvFile(FILE_TEST_VECTORS_HR, ios::app);
    //! are we using algorithm1 (0) or algorithm2 (1) ?
    int cipherNumber = 0;
    switch (pEstreamSettings->usageType) {
        case ESTREAM_DISTINGUISHER:

            //SHALL WE BALANCE TEST VECTORS?
            if (pEstreamSettings->ballancedTestVectors && (m_numVectors[0] >= pGlobals->settings->testVectors.setSize/2))
                cipherNumber = 1;
            else if (pEstreamSettings->ballancedTestVectors && (m_numVectors[1] >= pGlobals->settings->testVectors.setSize/2))
                cipherNumber = 0;
            else
                rndGen->getRandomFromInterval(1, &cipherNumber);
            m_numVectors[cipherNumber]++;
            //Signalize the correct value
            for (int output = 0; output < pGlobals->settings->circuit.sizeOutputLayer; output++) m_tvOutputs[output] = cipherNumber * 0xff;

            //generate the plaintext for stream
            if ((cipherNumber == 0 && pEstreamSettings->algorithm1 != ESTREAM_RANDOM) ||
                (cipherNumber == 1 && pEstreamSettings->algorithm2 != ESTREAM_RANDOM) ) {
                if (pGlobals->settings->testVectors.saveTestVectors == 1)
                    tvFile  << "(alg n." << ((cipherNumber==0)?pEstreamSettings->algorithm1:pEstreamSettings->algorithm2) << " - " << ((cipherNumber==0)?pEstreamSettings->alg1RoundsCount:pEstreamSettings->alg2RoundsCount) << " rounds): ";

                status = setupPlaintext();
                if (status != STAT_OK) return status;
                status = m_encryptorDecryptor->encrypt(m_plaintextIn,m_tvInputs,cipherNumber,0);
                if (status != STAT_OK) return status;
                status = m_encryptorDecryptor->decrypt(m_tvInputs,m_plaintextOut,cipherNumber,1);
                if (status != STAT_OK) return status;

                // check if plaintext = encrypted-decrypted plaintext
                for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) {
                    if (m_plaintextOut[input] != m_plaintextIn[input]) {
                        mainLogger.out(LOGGER_ERROR) << "Decrypted text doesn't match the input. See " << FILE_TEST_VECTORS_HR << " for details." << endl;
                        if (pGlobals->settings->testVectors.saveTestVectors) {
                            tvFile << "### ERROR: PLAINTEXT-ENCDECTEXT MISMATCH!" << endl;
                        }
                        status = STAT_PROJECT_ERROR;
                        break;
                    }
                }
            }
            else { // RANDOM
                if (pGlobals->settings->testVectors.saveTestVectors == 1)
                    tvFile << "(RANDOM INPUT - " << rndGen->shortDescription() << "):";
                for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) {
                    rndGen->getRandomFromInterval(255, &m_plaintextOut[input]);
                    m_plaintextIn[input] = m_tvInputs[input] = m_plaintextOut[input];
                }
            }
            break;

    case ESTREAM_BITS_TO_CHANGE:
        status = setupPlaintext();
        if (status != STAT_OK) return status;

        // WE NEED TO LET EVALUATOR KNOW THE INPUTS
        for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++)
            m_tvOutputs[input] = m_tvInputs[input];

        break;

    default:
        mainLogger.out(LOGGER_ERROR) << "unknown usage type (" << pEstreamSettings->usageType << ") in " << shortDescription() << endl;
        return STAT_INVALID_ARGUMETS;
        break;
    }

    // save human-readable test vector
    if (pGlobals->settings->testVectors.saveTestVectors) {
        int tvg = 0;
        if (cipherNumber == 0) tvg = pEstreamSettings->algorithm1;
        else tvg = pEstreamSettings->algorithm2;
        tvFile << setfill('0');

        if (memcmp(m_tvInputs,m_plaintextIn,pGlobals->settings->testVectors.inputLength) != 0) {
            for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++)
            tvFile << setw(2) << hex << (int)(m_plaintextIn[input]);
            tvFile << "::";
        }

        for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++)
            tvFile << setw(2) << hex << (int)(m_tvInputs[input]);

        if (memcmp(m_tvInputs,m_plaintextOut,pGlobals->settings->testVectors.inputLength) != 0) {
            tvFile << "::";
            for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++)
            tvFile << setw(2) << hex << (int)(m_plaintextOut[input]);
        }
        tvFile << endl;
    }
    tvFile.close();

    return status;
}

int EstreamProject::generateCipherDataStream() {
    //! are we using algorithm1 (0) or algorithm2 (1) ?
    // TBD/TODO generate for both streams, see project_sha3
    int cipherNumber = -1;
    int algorithm = -1;
    int numRounds = -1;
    if (pEstreamSettings->algorithm1 != ESTREAM_RANDOM) {
        cipherNumber = 0;
        algorithm = pEstreamSettings->algorithm1;
        numRounds = pEstreamSettings->limitAlgRounds ? pEstreamSettings->alg1RoundsCount : -1;
    } else if (pEstreamSettings->algorithm2 != ESTREAM_RANDOM) {
        cipherNumber = 1;
        algorithm = pEstreamSettings->algorithm2;
        numRounds = pEstreamSettings->limitAlgRounds ? pEstreamSettings->alg2RoundsCount : -1;
    }
    if (cipherNumber == -1) {
        mainLogger.out(LOGGER_ERROR) << "Cannot generate stream from random, cipher must be specified." << endl;
        return STAT_INVALID_ARGUMETS;
    }
    mainLogger.out(LOGGER_INFO) << "Generating stream for " << estreamToString(algorithm);
    if (numRounds == -1) {
        mainLogger.out() << " (unlimitted version)" << endl;
    } else {
        mainLogger.out() << " (" << numRounds << " rounds)" << endl;
    }

    int status = STAT_OK;

    if (pEstreamSettings->cipherInitializationFrequency == ESTREAM_INIT_CIPHERS_ONCE) {
        status = m_encryptorDecryptor->setupKey();
        if (status != STAT_OK) return status;
        m_encryptorDecryptor->setupIV();
        if (status != STAT_OK) return status;
    }

    unsigned long alreadyGenerated = 0;
    while (pEstreamSettings->streamSize == 0 ? true : alreadyGenerated <= pEstreamSettings->streamSize) {
        if (pEstreamSettings->cipherInitializationFrequency == ESTREAM_INIT_CIPHERS_FOR_SET) {
            status = m_encryptorDecryptor->setupKey();
            if (status != STAT_OK) return status;
            m_encryptorDecryptor->setupIV();
            if (status != STAT_OK) return status;
        }
        for (int testVectorNumber = 0; testVectorNumber < pGlobals->settings->testVectors.setSize; testVectorNumber++) {
            if (status != STAT_OK) break;
            if (pEstreamSettings->cipherInitializationFrequency == ESTREAM_INIT_CIPHERS_FOR_VECTOR) {
                status = m_encryptorDecryptor->setupKey();
                if (status != STAT_OK) return status;
                m_encryptorDecryptor->setupIV();
                if (status != STAT_OK) return status;
            }

            status = setupPlaintext();
            if (status != STAT_OK) return status;
            status = m_encryptorDecryptor->encrypt(m_plaintextIn,m_tvInputs,cipherNumber,0);
            if (status != STAT_OK) return status;
            status = m_encryptorDecryptor->decrypt(m_tvInputs,m_plaintextOut,cipherNumber,1);
            if (status != STAT_OK) return status;

            // check if plaintext = encrypted-decrypted plaintext
            for (int input = 0; input < pGlobals->settings->testVectors.inputLength; input++) {
                if (m_plaintextOut[input] != m_plaintextIn[input]) {
                    status = STAT_PROJECT_ERROR;
                    mainLogger.out(LOGGER_ERROR) << "Decrypted text doesn't match the input." << endl;
                    break;
                }
            }
            for (int index = 0; index < pGlobals->settings->testVectors.inputLength; index++) {
                cout << m_tvInputs[index];
            }
            alreadyGenerated += pGlobals->settings->testVectors.inputLength;
        }
    }
    mainLogger.out(LOGGER_INFO) << "Cipher data stream generation ended. (" << alreadyGenerated << ")" << endl;

    return status;
}