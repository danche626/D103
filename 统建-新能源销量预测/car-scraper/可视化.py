import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.model_selection import train_test_split, GridSearchCV
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import ConvLSTM2D, Flatten, Dense, Dropout
import xgboost as xgb
from sklearn.metrics import mean_squared_error, mean_absolute_percentage_error
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os
import time
from datetime import datetime

# 清除matplotlib字体缓存
# 删除以下行
# import matplotlib
# matplotlib.font_manager._rebuild()

# 设置字体
try:
    # 直接使用字体名称
    plt.rcParams['font.family'] = 'SimHei'
    plt.rcParams['axes.unicode_minus'] = False
except:
    # 如果找不到SimHei字体，尝试使用其他中文字体
    font_list = [f.name for f in fm.fontManager.ttflist if 'Hei' in f.name or 'hei' in f.name]
    if font_list:
        plt.rcParams['font.family'] = font_list[0]
    else:
        print("警告：未找到合适的中文字体，图表可能无法正确显示中文")

# 读取CSV数据
def read_data(file_path):
    data = pd.read_csv(file_path)
    return data

# 数据预处理
def preprocess_data(data):
    # 处理缺失值
    data = data.ffill()

    # 增加时间特征
    if 'date' in data.columns:
        data['date'] = pd.to_datetime(data['date'])
        data['year'] = data['date'].dt.year
        data['month'] = data['date'].dt.month
        data['day'] = data['date'].dt.day

    # 处理异常值
    Q1 = data['sales'].quantile(0.25)
    Q3 = data['sales'].quantile(0.75)
    IQR = Q3 - Q1
    data = data[(data['sales'] >= Q1 - 1.5 * IQR) & (data['sales'] <= Q3 + 1.5 * IQR)]

    # 分离特征和目标变量
    X = data.drop('sales', axis=1)
    y = data['sales']

    # 处理分类变量
    categorical_cols = X.select_dtypes(include=['object']).columns
    encoder = OneHotEncoder(sparse_output=False)
    X_categorical = encoder.fit_transform(X[categorical_cols])
    
    # 获取数值特征
    X_numerical = X.drop(categorical_cols, axis=1)
    
    # 如果有数值特征，则进行标准化
    if not X_numerical.empty:
        scaler = StandardScaler()
        X_numerical = scaler.fit_transform(X_numerical)
    else:
        # 如果没有数值特征，创建一个空数组
        X_numerical = np.empty((X.shape[0], 0))

    # 合并特征
    X = np.hstack([X_numerical, X_categorical])

    return X, y

# 构建ConvLSTM模型
def build_convlstm_model(input_shape):
    model = Sequential()
    model.add(ConvLSTM2D(filters=128, kernel_size=(3, 3), input_shape=input_shape, padding='same', return_sequences=True))
    model.add(ConvLSTM2D(filters=64, kernel_size=(3, 3), padding='same', return_sequences=False))
    model.add(Flatten())
    model.add(Dropout(0.2))
    model.add(Dense(64, activation='relu'))
    model.add(Dropout(0.2))
    model.add(Dense(1))
    model.compile(optimizer='adam', loss='mse')
    return model

# 构建XGBoost模型并使用网格搜索优化
def build_xgboost_model():
    model = xgb.XGBRegressor()
    param_grid = {
        'n_estimators': [200, 300, 400],
        'learning_rate': [0.01, 0.05, 0.1],
        'max_depth': [4, 5, 6],
        'subsample': [0.8, 0.9, 1.0],
        'colsample_bytree': [0.8, 0.9, 1.0]
    }
    grid_search = GridSearchCV(model, param_grid, cv=3, scoring='neg_mean_squared_error')
    return grid_search

# 主函数
def main():
    # 创建结果文件夹
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    result_dir = f"results_{timestamp}"
    os.makedirs(result_dir, exist_ok=True)

    file_path = '/Users/ruanji/Desktop/统建/car-scraper/byd_sales.csv'
    desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")

    best_mse = float('inf')
    best_mape = float('inf')
    best_params = None
    best_y_test = None
    best_y_pred = None

    for i in range(20):
        print(f"\n正在运行第 {i+1} 次...")
        start_time = time.time()
        
        data = read_data(file_path)
        X, y = preprocess_data(data)
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42+i)
        X_train_convlstm = X_train.reshape(X_train.shape[0], 1, X_train.shape[1], 1, 1)
        X_test_convlstm = X_test.reshape(X_test.shape[0], 1, X_test.shape[1], 1, 1)
        
        convlstm_model = build_convlstm_model(input_shape=(1, X_train.shape[1], 1, 1))
        convlstm_model.fit(X_train_convlstm, y_train, epochs=20, batch_size=32, verbose=0)
        
        convlstm_features_train = convlstm_model.predict(X_train_convlstm)
        convlstm_features_test = convlstm_model.predict(X_test_convlstm)
        
        X_train_combined = np.hstack([X_train, convlstm_features_train])
        X_test_combined = np.hstack([X_test, convlstm_features_test])
        
        xgboost_model = build_xgboost_model()
        xgboost_model.fit(X_train_combined, y_train)
        
        y_pred = xgboost_model.predict(X_test_combined)
        
        mse = mean_squared_error(y_test, y_pred)
        mape = mean_absolute_percentage_error(y_test, y_pred)
        
        # 更新最佳结果
        if mse < best_mse:
            best_mse = mse
            best_mape = mape
            best_params = xgboost_model.best_params_
            best_y_test = y_test
            best_y_pred = y_pred

        # 保存结果
        result = {
            'run_number': i+1,
            'best_params': xgboost_model.best_params_,
            'mse': mse,
            'mape': mape,
            'y_test': y_test,
            'y_pred': y_pred
        }
        
        # 保存到文件
        result_file = os.path.join(result_dir, f"run_{i+1}.npz")
        np.savez(result_file, **result)
        
        # 打印进度
        elapsed_time = time.time() - start_time
        print(f"第 {i+1} 次运行完成，耗时 {elapsed_time:.2f} 秒")
        print(f"结果已保存到 {result_file}")

        # 可视化
        plt.figure(figsize=(10, 6))
        plt.plot(y_test.values, label='real')
        plt.plot(y_pred, label='forecast')
        plt.title('real vs forecast')
        plt.xlabel('sample')
        plt.ylabel('sales')
        plt.legend()

        # 保存图表到桌面
        chart_file = os.path.join(desktop_path, f"run_{i+1}_chart.png")
        plt.savefig(chart_file)
        plt.close()

        print(f"第 {i+1} 次运行的图表已保存到 {chart_file}")

    print(f"\n所有运行完成，结果保存在 {result_dir} 目录中")

    # 输出最佳参数
    print("最佳参数:", best_params)

    # 评估最佳模型
    print(f"最佳均方误差 (MSE): {best_mse}")
    print(f"最佳平均绝对百分比误差 (MAPE): {best_mape}")

    # 可视化最佳结果
    plt.figure(figsize=(10, 6))
    plt.plot(best_y_test.values, label='real')
    plt.plot(best_y_pred, label='forecast')
    plt.title('Best real vs forecast')
    plt.xlabel('sample')
    plt.ylabel('sales')
    plt.legend()

    # 保存最佳图表到桌面
    best_chart_file = os.path.join(desktop_path, "best_chart.png")
    plt.savefig(best_chart_file)
    plt.close()

    print(f"最佳运行的图表已保存到 {best_chart_file}")

if __name__ == "__main__":
    main()