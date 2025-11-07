package com.linux.permissionmanager.fragment;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ProgressDialog;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.linux.permissionmanager.AppSettings;
import com.linux.permissionmanager.R;
import com.linux.permissionmanager.adapter.SelectFileRecyclerAdapter;
import com.linux.permissionmanager.bridge.NativeBridge;
import com.linux.permissionmanager.helper.SelectAppDlg;
import com.linux.permissionmanager.helper.SelectFileDlg;
import com.linux.permissionmanager.model.PopupWindowOnTouchClose;
import com.linux.permissionmanager.model.SelectAppItem;
import com.linux.permissionmanager.model.SelectFileItem;
import com.linux.permissionmanager.utils.ClipboardUtils;
import com.linux.permissionmanager.utils.DialogUtils;
import com.linux.permissionmanager.utils.ScreenInfoUtils;

import org.json.JSONArray;
import org.json.JSONObject;

import java.net.URLDecoder;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class HomeFragment extends Fragment implements View.OnClickListener {
    private Activity mActivity;
    private String mRootKey = "";
    private String lastInputCmd = "id";
    private String lastInputRootExecPath = "";
    private ProgressDialog m_loadingDlg = null;

    private EditText console_edit;

    public HomeFragment(Activity activity) {
        mActivity = activity;
    }
    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_home, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        lastInputCmd = AppSettings.getString("lastInputCmd", lastInputCmd);
        lastInputRootExecPath = AppSettings.getString("lastInputRootExecPath", lastInputRootExecPath);

        Button install_skroot_env_btn = view.findViewById(R.id.install_skroot_env_btn);
        Button uninstall_skroot_env_btn = view.findViewById(R.id.uninstall_skroot_env_btn);
        Button test_root_btn = view.findViewById(R.id.test_root_btn);
        Button run_root_cmd_btn = view.findViewById(R.id.run_root_cmd_btn);
        Button root_exec_process_btn = view.findViewById(R.id.root_exec_process_btn);
        Button implant_app_btn = view.findViewById(R.id.implant_app_btn);
        Button copy_info_btn = view.findViewById(R.id.copy_info_btn);
        Button clean_info_btn = view.findViewById(R.id.clean_info_btn);
        console_edit = view.findViewById(R.id.console_edit);

        install_skroot_env_btn.setOnClickListener(this);
        uninstall_skroot_env_btn.setOnClickListener(this);
        test_root_btn.setOnClickListener(this);
        run_root_cmd_btn.setOnClickListener(this);
        root_exec_process_btn.setOnClickListener(this);
        implant_app_btn.setOnClickListener(this);
        copy_info_btn.setOnClickListener(this);
        clean_info_btn.setOnClickListener(this);
    }

    public void setRootKey(String rootKey) {
        mRootKey = rootKey;
        showSkrootStatus();
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.install_skroot_env_btn:
                onClickInstallSkrootEnvBtn();
                break;
            case R.id.uninstall_skroot_env_btn:
                onClickUninstallSkrootEnvBtn();
                break;
            case R.id.test_root_btn:
                appendConsoleMsg(NativeBridge.testRoot(mRootKey));
                break;
            case R.id.run_root_cmd_btn:
                showInputRootCmdDlg();
                break;
            case R.id.root_exec_process_btn:
                showInputRootExecProcessPathDlg();
                break;
            case R.id.implant_app_btn:
                onClickImplantAppBtn();
                break;
            case R.id.copy_info_btn:
                copyConsoleMsg();
                break;
            case R.id.clean_info_btn:
                cleanConsoleMsg();
                break;
            default:
                break;
        }
    }
    private void showSkrootStatus() {
        String ver = NativeBridge.getInstalledSkrootEnvVersion(mRootKey);
        if(ver.isEmpty()) {
            appendConsoleMsg("SKRoot环境未安装！");
        } else  {
            appendConsoleMsg("SKRoot环境已安装，SDK版本：" + ver);
        }
    }

    private void showInputRootCmdDlg() {
        Handler inputCallback = new Handler() {
            @Override
            public void handleMessage(@NonNull Message msg) {
                String text = (String)msg.obj;
                lastInputCmd = text;
                AppSettings.setString("lastInputCmd", lastInputCmd);
                appendConsoleMsg(text + "\n" + NativeBridge.runRootCmd(mRootKey, text));
                super.handleMessage(msg);
            }
        };
        DialogUtils.showInputDlg(mActivity, lastInputCmd, "请输入ROOT命令", null, inputCallback, null);
    }

    private void onClickInstallSkrootEnvBtn() {
        appendConsoleMsg(NativeBridge.installSkrootEnv(mRootKey));
    }

    private void onClickUninstallSkrootEnvBtn() {
        DialogUtils.showCustomDialog(
                mActivity,
                "确认",
                "确定要卸载SKRoot环境吗？这会同时清空 SU 授权列表和删除已安装的模块",
                null,
                "确定", (dialog, which) -> {
                    dialog.dismiss();
                    appendConsoleMsg(NativeBridge.uninstallSkrootEnv(mRootKey));
                },
                "取消", (dialog, which) -> {
                    dialog.dismiss();
                }
        );
    }

    private void showInputRootExecProcessPathDlg() {
        Handler inputCallback = new Handler() {
            @Override
            public void handleMessage(@NonNull Message msg) {
                String text = (String)msg.obj;

                lastInputRootExecPath = text;
                AppSettings.setString("lastInputRootExecPath", lastInputRootExecPath);
                appendConsoleMsg(text + "\n" + NativeBridge.rootExecProcessCmd(mRootKey, text));
                super.handleMessage(msg);
            }
        };
        Handler helperCallback = new Handler() {
            @Override
            public void handleMessage(@NonNull Message msg) {
                DialogUtils.showMsgDlg(mActivity,"帮助", "请将JNI可执行文件放入/data内任意目录并且赋予777权限，如/data/app/com.xx，然后输入文件路径，即可直接执行，如：\n/data/com.xx/aaa\n", null);
                super.handleMessage(msg);
            }
        };
        DialogUtils.showInputDlg(mActivity, lastInputRootExecPath, "请输入Linux可运行文件的位置", "指导", inputCallback, helperCallback);
        DialogUtils.showMsgDlg(mActivity,"提示", "本功能是以ROOT身份直接运行程序，可避免产生su、sh等多余驻留后台进程，能最大程度上避免侦测", null);
    }

    private void onClickImplantAppBtn() {
        Handler selectImplantAppCallback = new Handler() {
            @Override
            public void handleMessage(@NonNull Message msg) {
                SelectAppItem app = (SelectAppItem) msg.obj;
                if (m_loadingDlg == null) {
                    m_loadingDlg = new ProgressDialog(mActivity);
                    m_loadingDlg.setCancelable(false);
                }
                m_loadingDlg.setTitle("");
                m_loadingDlg.setMessage("请现在手动启动APP [" + app.getShowName(mActivity) + "]");
                m_loadingDlg.show();
                startImplantAppThread(app);
                super.handleMessage(msg);
            }
        };
        View view = SelectAppDlg.showSelectAppDlg(mActivity, mRootKey, selectImplantAppCallback);
        CheckBox show_system_app_ckbox = view.findViewById(R.id.show_system_app_ckbox);
        CheckBox show_thirty_app_ckbox = view.findViewById(R.id.show_thirty_app_ckbox);
        CheckBox show_running_app_ckbox = view.findViewById(R.id.show_running_app_ckbox);
        show_system_app_ckbox.setChecked(false);
        show_system_app_ckbox.setEnabled(false);
        show_thirty_app_ckbox.setChecked(true);
        show_thirty_app_ckbox.setEnabled(false);
        show_running_app_ckbox.setChecked(true);
        show_running_app_ckbox.setEnabled(false);
    }

    private void startImplantAppThread(SelectAppItem app) {
        new Thread() {
            public void run() {
                String parasitePrecheckAppRet = NativeBridge.parasitePrecheckApp(mRootKey, app.getPackageName());

                mActivity.runOnUiThread(new Runnable() {
                    public void run() {
                        m_loadingDlg.cancel();
                        Map<String, SelectFileDlg.FileStatus> fileList = parseSoFullPathInfo(parasitePrecheckAppRet);
                        if (fileList.size() == 0 && !parasitePrecheckAppRet.isEmpty()) {
                            appendConsoleMsg(parasitePrecheckAppRet);
                            return;
                        }
                        Handler selectFileCallback = new Handler() {
                            @Override
                            public void handleMessage(@NonNull Message msg) {
                                SelectFileItem fileItem = (SelectFileItem) msg.obj;
                                String parasiteImplantAppRet = NativeBridge.parasiteImplantApp(mRootKey, app.getPackageName(), fileItem.getFilePath());
                                appendConsoleMsg(parasiteImplantAppRet);
                                if(parasiteImplantAppRet.indexOf("parasite_implant_app done.")!= -1) {
                                    DialogUtils.showMsgDlg(mActivity, "提示",
                                            "已经寄生到APP [" + app.getShowName(mActivity) + "]",
                                            app.getDrawable(mActivity));
                                }
                                super.handleMessage(msg);
                            }
                        };
                        SelectFileDlg.showSelectFileDlg(mActivity, fileList, selectFileCallback);
                    }
                });
            }
        }.start();
    }

    private void appendConsoleMsg(String msg) {
        StringBuffer txt = new StringBuffer();
        txt.append(console_edit.getText().toString());
        if (txt.length() != 0) txt.append("\n");
        txt.append(msg);
        txt.append("\n");
        console_edit.setText(txt.toString());
        console_edit.setSelection(txt.length());
    }

    private void copyConsoleMsg() {
        ClipboardUtils.copyText(mActivity, console_edit.getText().toString());
        Toast.makeText(mActivity, "复制成功", Toast.LENGTH_SHORT).show();
    }

    private void cleanConsoleMsg() {
        console_edit.setText("");
    }

    private Map<String, SelectFileDlg.FileStatus> parseSoFullPathInfo(String jsonStr) {
        Map<String, SelectFileDlg.FileStatus> soPathMap = new HashMap<>();
        try {
            JSONArray jsonArray = new JSONArray(jsonStr);
            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);
                String name = URLDecoder.decode(jsonObject.getString("name"), "UTF-8");
                int n = jsonObject.getInt("status");
                SelectFileDlg.FileStatus status =  SelectFileDlg.FileStatus.Unknown;
                status = n == 1 ?  SelectFileDlg.FileStatus.Running : status;
                status = n == 2 ?  SelectFileDlg.FileStatus.NotRunning : status;
                soPathMap.put(name, status);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return soPathMap;
    }
}
