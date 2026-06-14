import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.model_selection import train_test_split, GridSearchCV
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import ConvLSTM2D, Flatten, Dropout, Dense
import xgboost as xgb
from sklearn.metrics import mean_squared_error, mean_absolute_percentage_error
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os
import time
from datetime import datetime


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
    data = data.ffill()

    # 增加时间特征
    if 'date' in data.columns:
        # 明确指定日期格式
        data['date'] = pd.to_datetime(data['date'], format='%Y_%m')
        data['year'] = data['date'].dt.year
        data['month'] = data['date'].dt.month
        data['day'] = data['date'].dt.day

    # 引入1 - 3阶滞后项
    for i in range(1, 4):
        data[f'sales_lag_{i}'] = data['sales'].shift(i)

    # 引入季节性特征
    data['season'] = data['month'].apply(lambda x: (x % 12 + 3) // 3)

    # 引入趋势特征
    data['trend'] = data.index

    data = data.dropna()

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

    # 剔除datetime类型的列
    datetime_cols = X_numerical.select_dtypes(include=['datetime64']).columns
    X_numerical = X_numerical.drop(datetime_cols, axis=1)

    # 如果有数值特征，则进行标准化
    if not X_numerical.empty:
        scaler = StandardScaler()
        X_numerical = scaler.fit_transform(X_numerical)
    else:
        X_numerical = np.empty((X.shape[0], 0))

    # 合并特征
    X = np.hstack([X_numerical, X_categorical])

    return X, y


# 构建ConvLSTM模型（优化后）
def build_convlstm_model(input_shape):
    model = Sequential([
        ConvLSTM2D(filters=32, kernel_size=(1,3),  # 减少filters数量，缩小kernel_size
                  input_shape=input_shape,
                  padding='same',
                  return_sequences=True),
        ConvLSTM2D(filters=16, kernel_size=(1,3),  # 减少层数和filters
                  padding='same',
                  return_sequences=False),
        Flatten(),
        Dropout(0.3),
        Dense(32, activation='relu'),  # 减少神经元数量
        Dense(1)
    ])
    model.compile(optimizer='adam', loss='mse')
    return model

# 在 build_convlstm_model 和 build_xgboost_spatiotemporal_model 之间添加

# 构建XGBoost模型（优化后）
def build_xgboost_model():
    model = xgb.XGBRegressor(
        tree_method='hist',  # 使用CPU加速
        n_jobs=-1,           # 并行计算
        enable_categorical=True
    )
    param_grid = {
        'n_estimators': [50, 100],
        'learning_rate': [0.01, 0.02],
        'max_depth': [3, 4],
        'subsample': [0.8, 0.9],
        'colsample_bytree': [0.8, 0.9]
    }
    grid_search = GridSearchCV(model, param_grid, cv=3, scoring='neg_mean_squared_error')
    return grid_search

# 删除重复的 build_xgboost_spatiotemporal_model 定义（保留优化后的版本）

def build_xgboost_spatiotemporal_model():
    model = xgb.XGBRegressor(
        tree_method='hist',  # 改为CPU优化方法
        n_jobs=-1,
        enable_categorical=True,
        random_state=42  # 添加随机种子
    )
    param_grid = {
        'n_estimators': [100, 300],
        'learning_rate': [0.05, 0.1],
        'max_depth': [4, 6],
        'subsample': [0.8, 0.9],
        'colsample_bytree': [0.8, 0.9]
    }
    grid_search = GridSearchCV(model, param_grid, cv=3, scoring='neg_mean_squared_error')
    return grid_search


# 可视化结果（合并）
def visualize_combined_results(y_test, y_pred_xgboost, y_pred_xgboost_spatiotemporal, title, save_path):
    plt.figure(figsize=(12, 8))
    # 绘制真实值
    plt.plot(y_test.values, label='真实值', color='blue', linewidth=2, linestyle='-', marker='o', markersize=6)
    # 绘制XGBoost预测值
    plt.plot(y_pred_xgboost, label='XGBoost预测值', color='red', linewidth=2, linestyle='--', marker='s', markersize=6)
    # 绘制具有时空滞后性的XGBoost预测值
    plt.plot(y_pred_xgboost_spatiotemporal, label='XGBoost时空滞后预测值', color='green', linewidth=2, linestyle='-.', marker='^', markersize=6)

    # 添加数据标签
    for x, y in enumerate(y_test.values):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, 5), ha='center', fontsize=8)
    for x, y in enumerate(y_pred_xgboost):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, -5), ha='center', fontsize=8)
    for x, y in enumerate(y_pred_xgboost_spatiotemporal):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, -10), ha='center', fontsize=8)

    plt.title(title, fontsize=18)
    plt.xlabel('样本编号', fontsize=14)
    plt.ylabel('销量', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=14, loc='upper left')
    plt.tight_layout()
    plt.savefig(save_path)
    plt.close()
    print(f"图表已保存到 {save_path}")


# 主函数
def main():
    # 创建结果文件夹
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    result_dir = f"results_{timestamp}"
    os.makedirs(result_dir, exist_ok=True)

    file_path = '/Users/ruanji/Desktop/统建/数据/中南地区.csv'
    desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")

    best_mse = float('inf')
    best_mape = float('inf')
    best_params = None
    best_y_test = None
    best_y_pred_xgboost = None
    best_y_pred_xgboost_spatiotemporal = None

    for i in range(5):  # 修改为5个循环
        print(f"\n正在运行第 {i + 1} 次...")
        start_time = time.time()

        data = read_data(file_path)
        X, y = preprocess_data(data)
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42 + i)

        # 训练ConvLSTM模型
        convlstm_model = build_convlstm_model(input_shape=(1, X_train.shape[1], 1, 1))
        convlstm_model.fit(X_train.reshape(X_train.shape[0], 1, X_train.shape[1], 1, 1), y_train, epochs=20, batch_size=32, verbose=0)
        convlstm_features_train = convlstm_model.predict(X_train.reshape(X_train.shape[0], 1, X_train.shape[1], 1, 1))
        convlstm_features_test = convlstm_model.predict(X_test.reshape(X_test.shape[0], 1, X_test.shape[1], 1, 1))
        X_train_combined = np.hstack([X_train, convlstm_features_train])
        X_test_combined = np.hstack([X_test, convlstm_features_test])

        # 训练XGBoost模型
        xgboost_model = build_xgboost_model()
        xgboost_model.fit(X_train_combined, y_train)
        y_pred_xgboost = xgboost_model.predict(X_test_combined)

        # 训练具有时空滞后性的XGBoost模型，使用原始特征
        xgboost_spatiotemporal_model = build_xgboost_spatiotemporal_model()
        xgboost_spatiotemporal_model.fit(X_train, y_train)
        y_pred_xgboost_spatiotemporal = xgboost_spatiotemporal_model.predict(X_test)

        # 评估模型
        mse_xgboost = mean_squared_error(y_test, y_pred_xgboost)
        mape_xgboost = mean_absolute_percentage_error(y_test, y_pred_xgboost)

        mse_xgboost_spatiotemporal = mean_squared_error(y_test, y_pred_xgboost_spatiotemporal)
        mape_xgboost_spatiotemporal = mean_absolute_percentage_error(y_test, y_pred_xgboost_spatiotemporal)

        # 选择更优的模型参数
        if mse_xgboost_spatiotemporal < mse_xgboost:
            best_mse = mse_xgboost_spatiotemporal
            best_mape = mape_xgboost_spatiotemporal
            best_params = xgboost_spatiotemporal_model.best_params_
            best_y_test = y_test
            best_y_pred_xgboost = y_pred_xgboost
            best_y_pred_xgboost_spatiotemporal = y_pred_xgboost_spatiotemporal
        else:
            best_mse = mse_xgboost
            best_mape = mape_xgboost
            best_params = xgboost_model.best_params_
            best_y_test = y_test
            best_y_pred_xgboost = y_pred_xgboost
            best_y_pred_xgboost_spatiotemporal = y_pred_xgboost_spatiotemporal

        # 保存结果
        result = {
            'run_number': i + 1,
            'xgboost_params': xgboost_model.best_params_,
            'xgboost_mse': mse_xgboost,
            'xgboost_mape': mape_xgboost,
            'xgboost_spatiotemporal_params': xgboost_spatiotemporal_model.best_params_,
            'xgboost_spatiotemporal_mse': mse_xgboost_spatiotemporal,
            'xgboost_spatiotemporal_mape': mape_xgboost_spatiotemporal
        }

        # 保存到 CSV
        result_df = pd.DataFrame([result])
        result_path = os.path.join(result_dir, f'result_{i + 1}.csv')
        result_df.to_csv(result_path, index=False)

        # 可视化（合并）
        visualize_combined_results(y_test, y_pred_xgboost, y_pred_xgboost_spatiotemporal, f'第 {i + 1} 次运行结果对比',
                                   os.path.join(result_dir, f'combined_result_{i + 1}.png'))

        end_time = time.time()
        print(f"第 {i + 1} 次运行完成，耗时: {end_time - start_time:.2f} 秒")

    # 保存最佳结果
    best_result = {
        'best_mse': best_mse,
        'best_mape': best_mape,
        'best_params': best_params
    }
    best_result_df = pd.DataFrame([best_result])
    best_result_path = os.path.join(result_dir, 'best_result.csv')
    best_result_df.to_csv(best_result_path, index=False)
    visualize_combined_results(best_y_test, best_y_pred_xgboost, best_y_pred_xgboost_spatiotemporal, '最佳结果对比',
                               os.path.join(result_dir, 'best_combined_result.png'))


if __name__ == "__main__":
    main()