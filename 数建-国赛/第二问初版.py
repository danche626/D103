import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.tree import DecisionTreeRegressor
from scipy.stats import norm
import os
from mpl_toolkits.mplot3d import Axes3D
import seaborn as sns

plt.rcParams["font.family"] = ["Arial Unicode Unicode MS"]
plt.rcParams["axes.unicode_minus"] = False
plt.rcParams["axes.titlesize"] = 14
plt.rcParams["axes.labelsize"] = 12

def week_to_float(w):
    if pd.isna(w):
        return np.nan
    if isinstance(w, str):
        w = w.strip()
        if "w" in w:
            parts = w.split("w")
            if len(parts) == 2:
                try:
                    w0 = int(parts[0]) if parts[0] else 0
                    d = int(parts[1].replace("+", "")) if parts[1] else 0
                    return w0 + d / 7.0
                except (ValueError, TypeError):
                    return np.nan
    try:
        return float(w)
    except (ValueError, TypeError):
        return np.nan

def logit(p):
    p = np.clip(p, 1e-6, 1 - 1e-6)
    return np.log(p / (1 - p))

def calculate_recommended_week(beta_hat, sigma_hat, bmi, z_thr, w_min, w_max, q=0.90, step=0.05):
    ws = np.arange(w_min, w_max + step, step)
    Xg = np.column_stack([
        np.ones_like(ws), ws, 
        np.full_like(ws, bmi), 
        ws**2, 
        np.full_like(ws, bmi**2), 
        ws * bmi
    ])
    zhat = Xg @ beta_hat
    达标概率 = 1 - norm.cdf((z_thr - zhat) / sigma_hat)
    valid_weeks = ws[达标概率 >= q]
    
    if valid_weeks.size == 0:
        return (np.nan, 达标概率)
    return (valid_weeks[0], 达标概率)

def fit_model(df):
    X = np.column_stack([
        np.ones(len(df)), df["w"], df["b"],
        df["w"]**2, df["b"]** 2, df["w"] * df["b"]
    ])
    beta_hat = np.linalg.lstsq(X, df["z"], rcond=None)[0]
    z_pred = X @ beta_hat
    resid = df["z"] - z_pred
    sigma_hat = resid.std(ddof=X.shape[1])
    r_squared = 1 - (resid.var() / df["z"].var())
    return beta_hat, sigma_hat, r_squared

def get_bmi_groups(b_grid, w_star, n_groups=5):
    mask = ~np.isnan(w_star)
    if np.sum(mask) < 10 or len(b_grid[mask]) < 2:
        raise ValueError("有效数据不足，无法进行分组（至少需要10个有效样本）")
    
    tree = DecisionTreeRegressor(max_leaf_nodes=n_groups, random_state=0)
    tree.fit(b_grid[mask].reshape(-1, 1), w_star[mask])
    
    def extract_thresholds(tree):
        thresholds = []
        def dfs(node):
            if tree.tree_.feature[node] != -2:
                thresholds.append(tree.tree_.threshold[node])
                dfs(tree.tree_.children_left[node])
                dfs(tree.tree_.children_right[node])
        dfs(0)
        return sorted(thresholds) if thresholds else []
    
    thresholds = extract_thresholds(tree)
    b_min, b_max = np.min(b_grid[mask]), np.max(b_grid[mask])
    bounds = [round(b_min, 2)] + [round(t, 2) for t in thresholds] + [round(b_max, 2)]
    bounds = sorted(list(set(bounds)))
    return bounds

def generate_group_table(df, bounds, beta_hat, sigma_hat, z_thr, q):
    rows = []
    for i in range(len(bounds) - 1):
        lo, hi = bounds[i], bounds[i + 1]
        mid_bmi = (lo + hi) / 2.0
        w_rec, _ = calculate_recommended_week(
            beta_hat, sigma_hat, mid_bmi, z_thr,
            w_min=df["w"].min(), w_max=df["w"].max(), q=q
        )
        
        失败概率 = np.nan
        if not np.isnan(w_rec):
            Xc = np.array([1.0, w_rec, hi, w_rec**2, hi**2, w_rec * hi])
            zhat = Xc @ beta_hat
            失败概率 = norm.cdf((z_thr - zhat) / sigma_hat) * 100
        
        if i < len(bounds) - 2:
            sample_count = ((df["b"] >= lo) & (df["b"] < hi)).sum()
            bmi_range = f"[{lo}, {hi})"
        else:
            sample_count = ((df["b"] >= lo) & (df["b"] <= hi)).sum()
            bmi_range = f"[{lo}, {hi}]"
        
        rows.append({
            "组次": i + 1,
            "BMI区间": bmi_range,
            "组内中位BMI": round(mid_bmi, 2),
            f"推荐NIPT孕周(满足{q*100:.0f}%达标)": round(w_rec, 2) if not np.isnan(w_rec) else "无推荐",
            "推荐孕周下的失败率(%)": round(失败概率, 2) if not np.isnan(失败概率) else "-",
            "组内样本量": int(sample_count)
        })
    return pd.DataFrame(rows)

def plot_bmi_vs_week(b_grid, w_star, bounds, q, save_path="q2_figA.png"):
    plt.figure(figsize=(10, 6))
    plt.plot(b_grid, w_star, color="#2c7fb8", linewidth=2.5, label="推荐最早达标孕周")
    for i, t in enumerate(bounds[1:-1]):
        plt.axvline(t, linestyle="--", color="#d95f02", alpha=0.7, 
                   label=f"分组边界{i+1}" if i == 0 else "")
    
    valid_idx = ~np.isnan(w_star)
    if np.sum(valid_idx) > 0:
        sample_idx = np.random.choice(
            np.where(valid_idx)[0], 
            min(200, np.sum(valid_idx)), 
            replace=False
        )
        plt.scatter(b_grid[sample_idx], w_star[sample_idx], color="#7fcdbb", 
                   alpha=0.5, s=15, label="数据点")
    
    plt.title(f"BMI与推荐最早达标孕周关系（达标概率≥{q*100:.0f}%）")
    plt.xlabel("BMI")
    plt.ylabel("推荐最早孕周（周）")
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(save_path, dpi=200)
    plt.close()

def plot_risk_heatmap(df, beta_hat, sigma_hat, z_thr, bounds, save_path="失败概率热力图.png"):
    w_grid = np.linspace(df["w"].min(), df["w"].max(), 60)
    b_grid2 = np.linspace(np.percentile(df["b"], 5), np.percentile(df["b"], 95), 60)
    WW, BB = np.meshgrid(w_grid, b_grid2)
    
    Xmesh = np.column_stack([
        np.ones(WW.size), WW.ravel(), BB.ravel(),
        (WW**2).ravel(), (BB**2).ravel(), (WW*BB).ravel()
    ])
    Zhat = Xmesh @ beta_hat
    失败概率 = norm.cdf((z_thr - Zhat) / sigma_hat).reshape(WW.shape)
    
    plt.figure(figsize=(10, 7))
    sns.heatmap(失败概率, cmap="viridis", alpha=0.9, 
                xticklabels=[f"{w:.1f}" for w in w_grid[::5]],
                yticklabels=[f"{b:.1f}" for b in b_grid2[::5]],
                cbar_kws={'label': '失败概率 P(Y<4%)'})
    
    for level, ls, color in [(0.05, '-', 'white'), (0.10, '--', '#ffeda0')]:
        contour = plt.contour(WW, BB, 失败概率, levels=[level], 
                             colors=[color], linewidths=2, linestyles=[ls],
                             origin='lower', extent=[WW.min(), WW.max(), BB.min(), BB.max()])
        plt.clabel(contour, inline=True, fmt={level: f"{level*100:.0f}%"}, fontsize=10)
    
    for t in bounds[1:-1]:
        plt.axhline(y=np.argmin(np.abs(b_grid2 - t)), linestyle="--", color="#d95f02", alpha=0.7)
    
    plt.title("孕周×BMI 失败概率热力图（含分组边界）")
    plt.xlabel("孕周数（周）")
    plt.ylabel("BMI")
    plt.tight_layout()
    plt.savefig(save_path, dpi=200)
    plt.close()

def plot_risk_3d_surface(df, beta_hat, sigma_hat, z_thr, bounds, save_path="失败概率3D表面图.png"):
    w_grid = np.linspace(df["w"].min(), df["w"].max(), 60)
    b_grid2 = np.linspace(np.percentile(df["b"], 5), np.percentile(df["b"], 95), 60)
    WW, BB = np.meshgrid(w_grid, b_grid2)
    
    Xmesh = np.column_stack([
        np.ones(WW.size), WW.ravel(), BB.ravel(),
        (WW**2).ravel(), (BB**2).ravel(), (WW*BB).ravel()
    ])
    Zhat = Xmesh @ beta_hat
    失败概率 = norm.cdf((z_thr - Zhat) / sigma_hat).reshape(WW.shape)
    
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    surf = ax.plot_surface(WW, BB, 失败概率, cmap="plasma", alpha=0.9,
                          linewidth=0.3, edgecolor='gray')
    
    ax.contour(WW, BB, 失败概率, levels=[0.05], zdir='z', offset=0.05,
              colors='white', linewidths=2)
    
    ax.view_init(elev=30, azim=45)
    
    for t in bounds[1:-1]:
        ax.plot([WW.min(), WW.max()], [t, t], [0, 0], linestyle="--", color="#d95f02", alpha=0.7)
    
    ax.set_title("孕周×BMI 失败概率3D表面图")
    ax.set_xlabel("孕周数（周）")
    ax.set_ylabel("BMI")
    ax.set_zlabel("失败概率 P(Y<4%)")
    fig.colorbar(surf, ax=ax, shrink=0.5, aspect=10, label='失败概率')
    
    plt.tight_layout()
    plt.savefig(save_path, dpi=200)
    plt.close()

def plot_risk_scatter(df, beta_hat, sigma_hat, z_thr, bounds, save_path="失败概率散点图.png"):
    np.random.seed(42)
    n_points = 1000
    w_sample = np.random.uniform(df["w"].min(), df["w"].max(), n_points)
    b_sample = np.random.uniform(np.percentile(df["b"], 5), np.percentile(df["b"], 95), n_points)
    
    X_sample = np.column_stack([
        np.ones(n_points), w_sample, b_sample,
        w_sample**2, b_sample**2, w_sample * b_sample
    ])
    Zhat_sample = X_sample @ beta_hat
    失败概率 = norm.cdf((z_thr - Zhat_sample) / sigma_hat)
    
    plt.figure(figsize=(10, 7))
    scatter = plt.scatter(w_sample, b_sample, c=失败概率, 
                         s=50 * 失败概率 + 10,
                         cmap="coolwarm", alpha=0.7, 
                         edgecolors='black', linewidth=0.5)
    
    w_grid = np.linspace(df["w"].min(), df["w"].max(), 100)
    for level, ls, color in [(0.05, '-', 'black'), (0.10, '--', 'gray')]:
        b_threshold = []
        for w in w_grid:
            def func(b):
                X = np.array([1, w, b, w**2, b**2, w*b])
                zhat = X @ beta_hat
                return norm.cdf((z_thr - zhat)/sigma_hat) - level
            
            try:
                low, high = df["b"].min(), df["b"].max()
                if func(low) * func(high) >= 0:
                    continue
                for _ in range(50):
                    mid = (low + high) / 2
                    if func(mid) < 0:
                        high = mid
                    else:
                        low = mid
                b_threshold.append((w, (low + high)/2))
            except:
                continue
        
        if b_threshold:
            b_threshold = np.array(b_threshold)
            plt.plot(b_threshold[:,0], b_threshold[:,1], ls=ls, color=color, 
                    label=f"{level*100:.0f}% 失败概率边界")
    
    for t in bounds[1:-1]:
        plt.axhline(t, linestyle="--", color="#d95f02", alpha=0.7)
    
    plt.colorbar(scatter, label='失败概率 P(Y<4%)')
    plt.title("孕周×BMI 失败概率散点图（颜色和大小表示概率）")
    plt.xlabel("孕周数（周）")
    plt.ylabel("BMI")
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(save_path, dpi=200)
    plt.close()

def plot_probability_curves(df, beta_hat, sigma_hat, z_thr, q, save_path="达标概率曲线.png"):
    plt.figure(figsize=(10, 6))
    b_quantiles = np.quantile(df["b"], [0.1, 0.5, 0.9])
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c"]
    labels = ["BMI 10%分位", "BMI 50%分位（中位）", "BMI 90%分位"]
    
    for b, color, label in zip(b_quantiles, colors, labels):
        ws = np.linspace(df["w"].min(), df["w"].max(), 150)
        Xg = np.column_stack([
            np.ones_like(ws), ws, np.full_like(ws, b),
            ws**2, np.full_like(ws, b**2), ws * b
        ])
        zhat = Xg @ beta_hat
        达标概率 = 1 - norm.cdf((z_thr - zhat) / sigma_hat)
        plt.plot(ws, 达标概率, linewidth=2.5, color=color, label=label)
    
    plt.axhline(q, linestyle="--", color="#9467bd", linewidth=2,
               label=f"目标达标概率 {q*100:.0f}%")
    
    plt.title("不同BMI分位的达标概率随孕周变化曲线")
    plt.xlabel("孕周数（周）")
    plt.ylabel("达标概率 P(Y≥4%)")
    plt.ylim(0, 1.05)
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(save_path, dpi=200)
    plt.close()

def main(excel_path="附件.xlsx", q=0.90, n_groups=5, output_dir="结果输出"):
    os.makedirs(output_dir, exist_ok=True)
    
    try:
        male = pd.read_excel(excel_path, sheet_name="男胎检测数据")
        male["BMI"] = male["体重"] / ((male["身高"] / 100.0) **2)
        male["孕周数"] = male["检测孕周"].apply(week_to_float)
        df = male[["孕周数", "BMI", "Y染色体浓度"]].dropna().copy()
        if df.empty:
            raise ValueError("预处理后的数据为空，请检查输入数据格式")
        
        df["Y染色体浓度"] = df["Y染色体浓度"].clip(1e-6, 1 - 1e-6)
        df["z"] = logit(df["Y染色体浓度"])
        df = df.rename(columns={"孕周数": "w", "BMI": "b"})
        
        beta_hat, sigma_hat, r_squared = fit_model(df)
        z_thr = logit(0.04)
        
        b_grid = np.linspace(np.percentile(df["b"], 5), np.percentile(df["b"], 95), 200)
        w_star = []
        for b in b_grid:
            w_val, _ = calculate_recommended_week(
                beta_hat, sigma_hat, b, z_thr, 
                df["w"].min(), df["w"].max(), q=q
            )
            w_star.append(w_val)
        w_star = np.array(w_star)
        
        if np.sum(~np.isnan(w_star)) < 10:
            raise ValueError("推荐孕周计算结果有效数据不足，无法继续分析")
        
        bounds = get_bmi_groups(b_grid, w_star, n_groups)
        group_table = generate_group_table(df, bounds, beta_hat, sigma_hat, z_thr, q)
        
        coef_df = pd.DataFrame({
            "变量": ["常数项", "孕周", "BMI", "孕周²", "BMI²", "孕周×BMI"],
            "系数估计": beta_hat.round(6)
        })
        metrics_df = pd.DataFrame({
            "模型指标": ["残差标准差(logit尺度)", "4%阈值对应的z值", 
                       f"模型R²（logit拟合）", "孕周范围", "BMI范围"],
            "数值": [
                round(sigma_hat, 6),
                round(z_thr, 6),
                round(r_squared, 4),
                f"{df['w'].min():.2f}-{df['w'].max():.2f}",
                f"{df['b'].min():.2f}-{df['b'].max():.2f}"
            ]
        })
        
        group_table.to_csv(f"{output_dir}/分组推荐结果.csv", index=False, encoding="utf-8-sig")
        coef_df.to_csv(f"{output_dir}/模型系数.csv", index=False, encoding="utf-8-sig")
        metrics_df.to_csv(f"{output_dir}/模型指标.csv", index=False, encoding="utf-8-sig")
        
        print("="*50)
        print("模型拟合指标：")
        print(metrics_df)
        print("\n分组推荐结果：")
        print(group_table)
        print("="*50)
        print(f"结果已保存至 {os.path.abspath(output_dir)}")
        
        plot_bmi_vs_week(b_grid, w_star, bounds, q, 
                        save_path=f"{output_dir}/BMI与推荐孕周关系.png")
        plot_risk_heatmap(df, beta_hat, sigma_hat, z_thr, bounds,
                         save_path=f"{output_dir}/失败概率热力图.png")
        plot_risk_3d_surface(df, beta_hat, sigma_hat, z_thr, bounds,
                           save_path=f"{output_dir}/失败概率3D表面图.png")
        plot_risk_scatter(df, beta_hat, sigma_hat, z_thr, bounds,
                         save_path=f"{output_dir}/失败概率散点图.png")
        plot_probability_curves(df, beta_hat, sigma_hat, z_thr, q,
                               save_path=f"{output_dir}/达标概率曲线.png")
        
    except FileNotFoundError:
        print(f"错误：未找到文件 {excel_path}，请检查路径是否正确")
    except Exception as e:
        print(f"分析过程出错：{str(e)}")

if __name__ == "__main__":
    main(
        excel_path=r'/Users/ruanji/Desktop/数建/附件.xlsx',
        q=0.90,
        n_groups=5,
        output_dir="NIPT分析结果"
    )