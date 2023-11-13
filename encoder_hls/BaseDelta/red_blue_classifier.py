"""
Rudimentary linear classifier meant to help precompute whether a given tile should be optimized in the red
or blue direction to save time
Work in Progress, was never fully implemented
"""

import pandas as pd

from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

from sklearn.linear_model import SGDClassifier

from sklearn.metrics import accuracy_score

red_blue_data = pd.read_csv("red_blue_ecc_data.csv")
x = red_blue_data.drop(['color'], axis=1)
y = red_blue_data['color']

trainX, testX, trainY, testY = train_test_split(x, y, test_size = 0.2)

scalar = StandardScaler()
scalar.fit(trainX)
trainX = scalar.transform(trainX)
testX = scalar.transform(testX)

clf = SGDClassifier()
clf.fit(trainX, trainY)

y_pred = clf.predict(testX)
print('Accuracy: {:.2f}'.format(accuracy_score(testY, y_pred)))

