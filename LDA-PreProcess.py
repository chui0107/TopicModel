import json
import logging
import nltk
import string
from nltk.corpus import stopwords
from nltk.stem.porter import PorterStemmer
from pydoc import Doc
import os
import collections

corpus = []

stopWordsCorpus = stopwords.words("english")

vocabSet = set()
vocabList = []

stemmer = PorterStemmer()

def TokenizeText(doc):
	
	global stopWords
	docList = []
	words = doc.split()
	for word in words:
		if word not in stopWordsCorpus and word not in string.punctuation:
			stemed = stemmer.stem(word)
			docList.append(stemed)
			vocabSet.add(stemed)
			
	if len(docList) > 0:
		corpus.append(docList)
	
def WriteVocabulary(fileName):
		
	with open(fileName, 'r') as f:
		
	 	try:
				
	 	 	categories = json.load(f)			
			#there are three categories, business, sport and entertainment
		  	for category in categories:
				for docs in categories[category]:
					for doc in docs:
						words = TokenizeText(doc)
						
		except:
			logging.exception('exception')
			

	global vocabList 
	for word in vocabSet:
		vocabList.append(word)
		
	vocabList.sort()
	
	vocabFile = os.path.dirname(fileName) + '/vocab.txt'
			
	with open(vocabFile, 'w') as f:
		for word in vocabList:
			f.write(word.encode("utf-8") + '\n')
				
def WriteDat(rawFile):
	
	global vocabList
	global corpus
	
	wordIndex = {}
	
	for i in xrange(len(vocabList)):
		wordIndex[vocabList[i]] = i
	
	docDat = os.path.dirname(rawFile) + '/vocab.dat'
	
	with open(docDat, 'w') as f:
		
		for doc in corpus:
			
			docDict = collections.defaultdict(lambda: 0)
			for word in doc:
				docDict[word] += 1
		
			docStr = str(len(docDict)) + ' '
			
			for word in docDict:
				docStr += str(wordIndex[word]) + ':' + str(docDict[word]) + ' '
		
			docStr = docStr.rstrip() + '\n'
			
			f.write(docStr.encode("utf-8"))
			
def main():
	
	rawFile = 'text/Words.txt'
	
	vocabFile = WriteVocabulary(rawFile)
	
	WriteDat(rawFile)
	
if __name__ == "__main__":
	main()