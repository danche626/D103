import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.model_selection import train_test_split, GridSearchCV
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import LSTM, Flatten, Dense, Dropout, Attention
from tensorflow.keras import Input, Model
import xgboost as xgb
from sklearn.metrics import mean_squared_error, mean_absolute_percentage_error
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os
import time
from datetime import datetime
from tensorflow.keras.callbacks import EarlyStopping

# 尝试直接指定字体文件
font_path = '/System/Library/AssetsV2/com_apple_MobileAsset_Font7/3419f2a427639ad8c8e139149a287865a90fa17e.asset/AssetData/PingFang.ttc'
if os.path.exists(font_path):
    font_prop = fm.FontProperties(fname=font_path)
    plt.rcParams['font.family'] = font_prop.get_name()
    plt.rcParams['axes.unicode_minus'] = False
    print("成功使用字体: PingFang SC")
else:
    print("警告：未找到PingFang字体，使用默认字体")

# 读取CSV数据
def read_data(file_path):
    data = pd.read_csv(file_path)
    return data

# 数据预处理
def preprocess_data(data):
    # 处理缺失值
    data = data.dropna()
    # 提取特征和标签
    X = data.drop('target', axis=1).values
    y = data['target'].values
    # 数据标准化
    scaler = StandardScaler()
    X = scaler.fit_transform(X)
    # 调整数据形状以适应LSTM输入
    X = np.reshape(X, (X.shape[0], 1, X.shape[1]))
    return X, y

# 构建LSTM模型
def build_lstm_model(input_shape):
    model = Sequential()
    model.add(LSTM(128, input_shape=input_shape, return_sequences=True))
    model.add(Dropout(0.2))
    model.add(LSTM(64, return_sequences=False))
    model.add(Dropout(0.2))
    model.add(Dense(1))
    model.compile(optimizer='adam', loss='mse')
    return model

# 构建具有注意力机制的LSTM模型
def build_lstm_attention_model(input_shape):
    inputs = Input(shape=input_shape)
    lstm_out = LSTM(128, return_sequences=True)(inputs)
    attention_out = Attention()([lstm_out, lstm_out])
    flatten_out = Flatten()(attention_out)
    dropout_out = Dropout(0.2)(flatten_out)
    outputs = Dense(1)(dropout_out)
    model = Model(inputs=inputs, outputs=outputs)
    model.compile(optimizer='adam', loss='mse')
    return model

# 构建XGBoost模型并使用网格搜索优化
def build_xgboost_model():
    model = xgb.XGBRegressor()
    param_grid = {
        'n_estimators': [100, 200, 300],
        'learning_rate': [0.01, 0.1, 0.2]
    }
    grid_search = GridSearchCV(model, param_grid, cv=3)
    return grid_search

# 构建具有时空滞后性的XGBoost模型
def build_xgboost_spatiotemporal_model():
    pass

# 可视化结果
def visualize_results(y_test, y_pred, title, save_path):
    plt.figure(figsize=(10, 6))
    plt.plot(y_test, label='真实值')
    plt.plot(y_pred, label='预测值')
    plt.title(title)
    plt.xlabel('样本')
    plt.ylabel('值')
    plt.legend()
    plt.savefig(save_path)
    plt.show()

# 主函数
def main():
    # 创建结果文件夹
    result_folder = 'results'
    if not os.path.exists(result_folder):
        os.makedirs(result_folder)

    # 读取数据
    file_path = 'your_data.csv'  # 替换为实际的数据文件路径
    data = read_data(file_path)

    # 数据预处理
    X, y = preprocess_data(data)

    # 划分训练集和测试集
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    # 构建并训练LSTM - Attention模型
    input_shape = (X_train.shape[1], X_train.shape[2])
    model = build_lstm_attention_model(input_shape)
    early_stopping = EarlyStopping(monitor='val_loss', patience=10, restore_best_weights=True)
    model.fit(X_train, y_train, epochs=100, batch_size=32, validation_split=0.1, callbacks=[early_stopping])

    # 预测
    y_pred = model.predict(X_test)

    # 可视化结果
    title = 'LSTM - Attention模型预测结果'
    save_path = os.path.join(result_folder, 'lstm_attention_result.png')
    visualize_results(y_test, y_pred, title, save_path)

if __name__ == "__main__":
    main()