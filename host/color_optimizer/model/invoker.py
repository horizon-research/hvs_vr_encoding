# Invoker: v0.0.1
# DO NOT MANUALLY EDIT THIS FILE.
#
# This script was generated with invoker.
# To regenerate file, run `invoker rebuild`.
# Date:	14/04/2022
# Hash:	0ba397868bef6a3280bd5726155b032c
import argparse
import copy
import importlib
import json
import logging
import logging.config
from pathlib import Path


class Module:
    def __init__(self, inp_args=None):
        all_args = self.args()
        # import ipdb; ipdb.set_trace()
        all_args.update(inp_args)
        conf = self.build_config(all_args)
        self.opt = _deserialize_config(conf)
        self.initialize()

    @classmethod
    def args(cls):
        return {}

    @classmethod
    def build_config(cls, args):
        return args

    def initialize(self):
        pass


class Script:
    def __init__(self, inp_args=None):
        # Parse Script Arguments
        self.all_args = _build_argparser(self.args(), inp_args)

    def initialize(self):
        conf = self.build_config(self.all_args)
        # Load Modules
        module_conf = {}
        for module, module_mode in self.modules().items():
            cls = importlib.import_module(module).get_class(module_mode)
            if module not in conf.keys():
                cls_inst = cls({})
            else:
                cls_inst = cls(conf[module])
            setattr(self, module, cls_inst)
            module_conf[module] = _serialize_opt(cls_inst.opt)
        conf.update(module_conf)
        # Initialize logger
        _init_logger()
        # Save Config
        if "path" in conf:
            save_root = Path(conf["path"])
            save_root.mkdir(parents=True, exist_ok=True)
            json.dump(
                {
                    "modules": self.modules(),
                    "config": conf,
                },
                open(save_root / "conf.json", "w"))
        # Deserialize Script Config
        self.opt = _deserialize_config(conf)
        return self

    @classmethod
    def args(cls):
        return {}

    @classmethod
    def modules(cls):
        return {}

    @classmethod
    def build_config(cls, args):
        return args

    def run(self):
        pass


class Workflow:
    def __init__(self):
        all_args = _build_argparser(self.args())
        self.arg_dict = self.build_script_args(all_args)

    @classmethod
    def args(cls):
        return {}

    @classmethod
    def scripts(cls):
        return []

    @classmethod
    def build_script_args(cls, args):
        arg_dict = {}
        for script in cls.scripts():
            arg_dict[script] = {}
        return arg_dict

    def run(self):
        for script in self.scripts():
            module = importlib.import_module(script)
            cls = getattr(module, _to_camel_case(script))
            cls_inst = cls().initialize()
            cls_inst.all_args = self.arg_dict[script]
            cls_inst.run()


def _init_logger():
    logger_dict = {
        "version": 1,
        "formatters": {
            "simple": {
                "format": "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
            }
        },
        "handlers": {
            "console": {
                "class": "logging.StreamHandler",
                "level": "DEBUG",
                "formatter": "simple",
                "stream": "ext://sys.stdout"
            }
        },
        "root": {
            "level": "ERROR",
            "handlers": ["console"]
        }
    }
    logging.config.dictConfig(logger_dict)


def _build_argparser(default_args, override_args=None):
    parser = argparse.ArgumentParser()
    for k, v in default_args.items():
        if type(v) == list:
            parser.add_argument(
                f"--{k}",
                type=type(v[0]) if len(v) > 0 else str,
                nargs="+",
                default=v
            )
        elif type(v) == bool:
            parser.add_argument(
                f"--{k}",
                action="store_true" if not v else "store_false",
            )
        else:
            parser.add_argument(
                f"--{k}",
                type=type(v),
                default=v
            )
    return vars(parser.parse_args(override_args))


def _serialize_opt(opt):
    out = vars(copy.deepcopy(opt))
    for k, v in out.items():
        if isinstance(v, argparse.Namespace):
            out[k] = _serialize_opt(v)
        else:
            out[k] = v
    return out


def _deserialize_config(config):
    opt = argparse.Namespace()
    for k, v in config.items():
        if isinstance(v, dict):
            setattr(opt, k, _deserialize_config(v))
        else:
            setattr(opt, k, v)
    return opt


def _to_camel_case(string):
    return "".join([token.capitalize() for token in string.split("_")])
